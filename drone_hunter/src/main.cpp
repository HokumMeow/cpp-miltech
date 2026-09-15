#include <iostream>
#include <cstring>
#include <cmath>
#include <memory>
#include <span>
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
};

std::string formatTestId(int n) {
    std::ostringstream oss;
    oss << 'T' << std::setw(2) << std::setfill('0') << n;
    return oss.str();
}

void reportResult(const std::string& dataPath, int testNumber) {
    const std::string testId = formatTestId(testNumber);
    const std::string path = dataPath + "/simulation.json";

    json simulation;
    try {
        std::ifstream fin(path);
        if (!fin) {
            std::cerr << "[report] не вдалося відкрити " << path << std::endl;
            return;
        }
        fin >> simulation;
    } catch (const std::exception& e) {
        std::cerr << "[report] не вдалося розпарсити " << path << ": " << e.what() << std::endl;
        return;
    }

    ResultReporter reporter(kReportBaseUrl, kReportApiKey, kReportMaxAttempts,
                             kReportRetryDelaySec, kReportTimeoutSec, kReportTimeoutSec);
    auto outcome = reporter.reportTest(kReportStudentId, testId, simulation);

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
        LOG("usage: drone_hunter <data_path> [analytical|table]\n");
        LOG("       drone_hunter --uart <dev> [--gpiochip <chip>] [--start-line <n>] [--drop-line <n>] [--data <path>] [analytical|table]\n");
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

    MissionProcessor mission(std::move(solver), std::move(loader), *provider, *physics);

    std::thread providerThread([&provider] { provider->run(); });
    std::thread physicsThread ([&physics]  { physics->run();  });
    std::thread missionThread (&MissionProcessor::run, &mission);

    while (!provider->isThreadReady() || !physics->isThreadReady() || !mission.isThreadReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    provider->start();
    physics->start();
    mission.start();

    missionThread.join();

    physics->stop();
    provider->stop();

    physicsThread.join();
    providerThread.join();

    if (args.remote) {
        uartLink->stop();
        uartThread.join();
    }

    mission.saveResults(args.dataPath);

    return 0;
}
