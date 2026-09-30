#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "failsafe/FailsafeFsm.h"
#include "logging/RouteLogger.h"
#include "nav/Navigator.h"
#include "source/HardwareTelemetrySource.h"
#include "source/ITelemetrySource.h"
#include "source/ReplayTelemetrySource.h"

namespace {

constexpr int64_t kPrintPeriodMs = 1000;

struct Options {
    bool hardware = false;
    std::string replayPath;
    std::string logPath;
    HardwareConfig hw;
    FailsafeConfig fs;
};

void printUsage() {
    std::cout <<
        "Usage:\n"
        "  rth_app --replay <scenario.csv> [--log <route.csv>]\n"
        "  rth_app --hardware [--log <route.csv>] [--i2c <dev>] [--gps <port>] [--gps-baud <n>]\n"
        "                     [--imu-addr <0x68>] [--baro-addr <0x77>] [--heading-offset <deg>]\n"
        "\n"
        "Failsafe settings (both modes):\n"
        "  --rth-alt <m>          altitude above home for the return leg (default 30)\n"
        "  --alt-tolerance <m>    how close to --rth-alt counts as 'reached' (default 2)\n"
        "  --link-timeout <sec>   silence before RTH starts (default 3)\n"
        "  --no-gps               do not wait for a GPS fix. If there is one, home is set as\n"
        "                         usual; if not, home is the barometer altitude only. Then there\n"
        "                         are no coordinates, so RTH stops at RETURN and waits for GPS.\n"
        "\n"
        "In --hardware mode: type 'l' + Enter to simulate loss/restore of the control link,\n"
        "'q' + Enter to quit.\n"
        "\n"
        "Indoor bench check (no GPS, lift the board by ~1 m):\n"
        "  rth_app --hardware --no-gps --rth-alt 1 --alt-tolerance 0.3\n";
}

bool parseArgs(int argc, char* argv[], Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const bool hasValue = i + 1 < argc;

        if (a == "--hardware") {
            opt.hardware = true;
        } else if (a == "--replay" && hasValue) {
            opt.replayPath = argv[++i];
        } else if (a == "--log" && hasValue) {
            opt.logPath = argv[++i];
        } else if (a == "--i2c" && hasValue) {
            opt.hw.i2cDevice = argv[++i];
        } else if (a == "--gps" && hasValue) {
            opt.hw.gpsPort = argv[++i];
        } else if (a == "--gps-baud" && hasValue) {
            opt.hw.gpsBaud = std::stoi(argv[++i]);
        } else if (a == "--imu-addr" && hasValue) {
            opt.hw.imuAddress = static_cast<uint8_t>(std::stoul(argv[++i], nullptr, 0));
        } else if (a == "--baro-addr" && hasValue) {
            opt.hw.baroAddress = static_cast<uint8_t>(std::stoul(argv[++i], nullptr, 0));
        } else if (a == "--heading-offset" && hasValue) {
            opt.hw.headingOffsetDeg = std::stod(argv[++i]);
        } else if (a == "--rth-alt" && hasValue) {
            opt.fs.rthAltitudeM = std::stod(argv[++i]);
        } else if (a == "--alt-tolerance" && hasValue) {
            opt.fs.altToleranceM = std::stod(argv[++i]);
        } else if (a == "--link-timeout" && hasValue) {
            opt.fs.linkTimeoutMs = static_cast<int64_t>(std::stod(argv[++i]) * 1000.0);
        } else if (a == "--no-gps") {
            opt.fs.requireGpsForHome = false;
        } else {
            return false;
        }
    }
    return opt.hardware != !opt.replayPath.empty();
}

double noMinusZero(double v) { return (v > -0.5 && v < 0.5) ? 0.0 : v; }

// рядок стану
// те, що прийшло з датчиків, GPS, зв'язок, і потім рекомендація навігації
void printStatus(const Telemetry& t, FlightState state, const NavCommand& cmd, double relAlt) {
    std::printf("[%6.1fs] %-9s hdg=%3.0f rp=%+3.0f/%+3.0f alt=%+6.1fm t=%4.1fC pos=%.5f,%.5f sats=%d link=%s",
                t.timeMs / 1000.0, toString(state), t.imu.headingDeg,
                noMinusZero(t.imu.rollDeg), noMinusZero(t.imu.pitchDeg),
                relAlt, t.baro.temperatureC, t.gps.latDeg, t.gps.lonDeg, t.gps.satellites,
                t.linkOk ? "ok" : "LOST");

    switch (cmd.action) {
        case NavAction::Climb:
            std::printf("  -> CLIMB %+.1fm", cmd.climbM);
            break;
        case NavAction::FlyHome:
            std::printf("  -> FLY_HOME dist=%.1fm bearing=%.0f turn=%+.0f climb=%+.1fm",
                        cmd.distanceToHomeM, cmd.bearingToHomeDeg, noMinusZero(cmd.headingErrorDeg), cmd.climbM);
            break;
        case NavAction::Land:
            std::printf("  -> LAND");
            break;
        case NavAction::Hold:
            std::printf("  -> HOLD (no GPS)");
            break;
        case NavAction::Manual:
            break;
    }
    std::printf("\n");
}

}  // namespace

int main(int argc, char* argv[]) {
    
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    Options opt;
    try {
        if (!parseArgs(argc, argv, opt)) {
            printUsage();
            return 1;
        }

        std::unique_ptr<ITelemetrySource> source;
        if (opt.hardware) {
            source = std::make_unique<HardwareTelemetrySource>(opt.hw);
        } else {
            source = std::make_unique<ReplayTelemetrySource>(opt.replayPath);
        }

        FailsafeFsm fsm(opt.fs);
        RouteLogger logger(opt.logPath);

        Telemetry t;
        FlightState prevState = fsm.state();
        int64_t lastPrintMs = -kPrintPeriodMs;
        bool homeReported = false;

        while (source->next(t)) {
            const FlightState state = fsm.update(t);

            if (fsm.hasHome() && !homeReported) {
                homeReported = true;
                if (fsm.homeHasPosition()) {
                    logger.setHome(fsm.home());
                    std::printf("Home set: %.6f, %.6f (alt %.1f m)\n",
                                fsm.home().latDeg, fsm.home().lonDeg, fsm.homeAltitudeM());
                } else {
                    // --no-gps: координат немає
                    std::printf("Home set: altitude only, %.1f m (no GPS fix)\n", fsm.homeAltitudeM());
                }
            }
            logger.log(t, state);

            if (state != prevState || t.timeMs - lastPrintMs >= kPrintPeriodMs) {
                if (state != prevState) {
                    std::printf("*** %s -> %s\n", toString(prevState), toString(state));
                }
                const double relAlt = fsm.hasHome() ? t.baro.altitudeM - fsm.homeAltitudeM() : 0.0;
                printStatus(t, state, computeNav(fsm, t), relAlt);
                lastPrintMs = t.timeMs;
                prevState = state;
            }
        }

        const RouteLogger::Stats& s = logger.stats();
        std::printf("\nSummary: %d samples, path %.1f m, max distance from home %.1f m, final state %s\n",
                    s.points, s.pathLengthM, s.maxDistanceFromHomeM, toString(fsm.state()));
    } catch (const std::exception& e) {
        std::cerr << "[fatal] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
