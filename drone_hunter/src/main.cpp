#include <iostream>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

#include "json.hpp"

#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "interfaces/IDronePhysics.h"
#include "interfaces/IConfigLoader.h"
#include "engine/MissionProcessor.h"
#include "engine/Factory.h"
#include "link/UartLink.h"
#include "link/GpioLink.h"
#include "report/ResultReporter.h"
#include "telemetry/MavlinkLink.h"
#include "Log.h"

using namespace std;
using json = nlohmann::json;

namespace {

constexpr const char* kReportBaseUrl = "http://cppmiltech.com.ua";
constexpr const char* kReportApiKey = "dz12-vX7mK4qT9r2w";
constexpr const char* kReportStudentId = "2028";
constexpr int kReportMaxAttempts = 5;
constexpr int kReportRetryDelaySec = 1;
constexpr int kReportTimeoutSec = 2;

struct CliArgs {
    std::string dataPath = "./data";
    SolverType solverType = SolverType::TABLE;
    bool remote = false;
    std::string uartDev;
    std::string gpiochip = "gpiochip1";
    unsigned startLine = 24;
    unsigned dropLine = 23;
    int testNumber = 0;  // 0 = without reporting; otherwise 1..10 → T01..T10
    std::string mavlinkHost = "127.0.0.1";
    std::uint16_t mavlinkPort = 14550;
};

std::string formatTestId(int n) {
    std::ostringstream oss;
    oss << 'T' << std::setw(2) << std::setfill('0') << n;
    return oss.str();
}

void reportResult(const std::string& dataPath, int testNumber) {
    const std::string testId = formatTestId(testNumber);
    const std::string path = dataPath + "/simulation.json";

    ResultReporter::Outcome outcome;
    outcome.testId = testId;

    json simulation;
    bool fileOk = false;
    std::ifstream fin(path);
    if (!fin) {
        outcome.status = ResultReporter::Status::FileMissing;
        outcome.message = "не вдалося відкрити " + path;
    } else {
        try {
            fin >> simulation;
            fileOk = true;
        } catch (const std::exception& e) {
            outcome.status = ResultReporter::Status::FileMissing;
            outcome.message = std::string("не вдалося розпарсити ") + path + ": " + e.what();
        }
    }

    if (fileOk) {
        ResultReporter reporter(kReportBaseUrl, kReportApiKey, kReportMaxAttempts,
                                 kReportRetryDelaySec, kReportTimeoutSec, kReportTimeoutSec);
        outcome = reporter.reportTest(kReportStudentId, testId, simulation);
    }

    const char* statusLabel = "?";
    switch (outcome.status) {
        case ResultReporter::Status::Success:     statusLabel = "OK"; break;
        case ResultReporter::Status::Unverified:  statusLabel = "OK (не підтверджено GET)"; break;
        case ResultReporter::Status::ClientError: statusLabel = "ПОМИЛКА ДАНИХ"; break;
        case ResultReporter::Status::ServerError: statusLabel = "НЕ ВДАЛОСЯ (сервер)"; break;
        case ResultReporter::Status::FileMissing: statusLabel = "ПРОПУЩЕНО"; break;
    }

    std::cout << "\n=== Звіт постингу результатів ===\n"
               << "Тест\tСпроби\tСтатус\n"
               << outcome.testId << '\t' << outcome.attempts << '\t' << statusLabel;
    if (outcome.status != ResultReporter::Status::Success && !outcome.message.empty()) {
        std::cout << "  (" << outcome.message << ")";
    }
    std::cout << std::endl;
}

// "127.0.0.1:14550" -> host="127.0.0.1", port=14550
bool parseMavlinkAddr(const std::string& addr, std::string& host, std::uint16_t& port) {
    const auto sep = addr.rfind(':');
    if (sep == std::string::npos || sep == 0 || sep + 1 == addr.size()) {
        std::cerr << "Invalid --mavlink address: " << addr << " (expected host:port)" << std::endl;
        return false;
    }
    host = addr.substr(0, sep);
    port = static_cast<std::uint16_t>(std::stoul(addr.substr(sep + 1)));
    return true;
}

bool parseArgs(std::span<char*> args, CliArgs& out) {
    std::vector<std::string> positional;
    bool dataPathSet = false;

    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string arg = args[i];
        if (arg == "--uart" && i + 1 < args.size()) {
            out.uartDev = args[++i];
            out.remote = true;
        } else if (arg == "--gpiochip" && i + 1 < args.size()) {
            out.gpiochip = args[++i];
        } else if (arg == "--start-line" && i + 1 < args.size()) {
            out.startLine = static_cast<unsigned>(std::stoul(args[++i]));
        } else if (arg == "--drop-line" && i + 1 < args.size()) {
            out.dropLine = static_cast<unsigned>(std::stoul(args[++i]));
        } else if (arg == "--data" && i + 1 < args.size()) {
            out.dataPath = args[++i];
            dataPathSet = true;
        } else if (arg == "--test" && i + 1 < args.size()) {
            out.testNumber = std::stoi(args[++i]);
        } else if (arg == "--mavlink" && i + 1 < args.size()) {
            if (!parseMavlinkAddr(args[++i], out.mavlinkHost, out.mavlinkPort)) {
                return false;
            }
        } else {
            positional.push_back(arg);
        }
    }

    std::size_t solverIdx = 0;
    if (!out.remote && !dataPathSet && !positional.empty()) {
        out.dataPath = positional[0];
        solverIdx = 1;
    }

    if (positional.size() > solverIdx) {
        const std::string& solverArg = positional[solverIdx];
        if (solverArg == "analytical") {
            out.solverType = SolverType::ANALYTICAL;
        } else if (solverArg == "table") {
            out.solverType = SolverType::TABLE;
        } else {
            std::cerr << "Unknown solver type: " << solverArg << " (expected 'analytical' or 'table')" << std::endl;
            return false;
        }
    }

    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    const auto kArgs = std::span<char*>(argv, static_cast<std::size_t>(argc));

    if (kArgs.size() < 2) {
        LOG("using default data path: ./data\n");
        LOG("usage: drone_hunter <data_path> [analytical|table] [--test <1..10>]\n");
        LOG("       drone_hunter --uart <dev> [--gpiochip <chip>] [--start-line <n>] [--drop-line <n>] [--data <path>] [--test <1..10>] [analytical|table]\n");
        LOG("       add [--mavlink <host:port>] to any form (default 127.0.0.1:14550)\n");
    }

    CliArgs args;
    if (!parseArgs(kArgs, args)) {
        return 1;
    }

    std::unique_ptr<UartLink> uartLink;
    std::unique_ptr<GpioLink> gpioLink;
    std::thread uartThread;

    std::unique_ptr<IConfigLoader> loader;

    if (args.remote) {
        uartLink = std::make_unique<UartLink>(args.uartDev);
        uartThread = std::thread([&uartLink] { uartLink->run(); });
        while (!uartLink->isThreadReady()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        uartLink->start();

        const std::string chipPath = "/dev/" + args.gpiochip;
        gpioLink = std::make_unique<GpioLink>(chipPath.c_str(), args.startLine, args.dropLine);
        gpioLink->raiseStart();

        loader = createConfigLoader(ConfigLoaderType::UART, args.dataPath, uartLink.get());
    } else {
        loader = createConfigLoader(ConfigLoaderType::FILE, args.dataPath);
    }

    if (!loader) {
        std::cerr << "Failed to create config loader" << std::endl;
        return 1;
    }
    loader->load();
    const DroneConfig cfg = loader->getConfig();

    auto provider = args.remote
        ? createProvider(ProviderType::UART, args.dataPath, cfg.arrayTimeStep,
                          cfg.targetTimeStep, cfg.timeScale, uartLink.get())
        : createProvider(ProviderType::JSON, args.dataPath, cfg.arrayTimeStep,
                          cfg.targetTimeStep, cfg.timeScale);
    if (!provider) {
        std::cerr << "Failed to create target provider" << std::endl;
        return 1;
    }

    auto physics = args.remote
        ? createPhysics(PhysicsType::REMOTE, cfg.startPos, cfg.initialDir,
                         cfg.attackSpeed, cfg.accelPath, cfg.physicsTimeStep, cfg.timeScale,
                         uartLink.get(), gpioLink.get())
        : createPhysics(PhysicsType::SIMULATED, cfg.startPos, cfg.initialDir,
                         cfg.attackSpeed, cfg.accelPath, cfg.physicsTimeStep, cfg.timeScale);
    if (!physics) {
        std::cerr << "Failed to create drone physics" << std::endl;
        return 1;
    }

    auto solver = createSolver(args.solverType, args.dataPath);
    if (!solver) {
        std::cerr << "Failed to create ballistic solver" << std::endl;
        return 1;
    }

    // telemetry MAVLink 2/UDP
    MavlinkLink mavlink(*physics, args.mavlinkHost, args.mavlinkPort, cfg.altitude);

    MissionProcessor mission(std::move(solver), std::move(loader), *provider, *physics, &mavlink);

    std::thread providerThread([&provider] { provider->run(); });
    std::thread physicsThread ([&physics]  { physics->run();  });
    std::thread mavlinkThread ([&mavlink]  { mavlink.run();   });
    std::thread missionThread (&MissionProcessor::run, &mission);

    while (!provider->isThreadReady() || !physics->isThreadReady() ||
           !mavlink.isThreadReady() || !mission.isThreadReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    provider->start();
    physics->start();
    mavlink.start();
    mission.start();

    missionThread.join();

    // Wait for drop commands up to 5 attempts
    const auto dropDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (mavlink.hasPendingDrop() && std::chrono::steady_clock::now() < dropDeadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    physics->stop();
    provider->stop();
    mavlink.stop();

    physicsThread.join();
    providerThread.join();
    mavlinkThread.join();

    if (args.remote) {
        uartLink->stop();
        uartThread.join();
    }

    mission.saveResults(args.dataPath);

    if (args.testNumber != 0) {
        reportResult(args.dataPath, args.testNumber);
    }

    return 0;
}
