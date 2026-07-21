#include <thread>
#include "config/UartConfigLoader.h"

void UartConfigLoader::load() {
    
    while (!(link_.hasAmmo() && link_.hasConfig() && link_.hasTelemetry())) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
        
    dlink::AmmoCfg ammoCfg = link_.getAmmoCfg();
    dlink::DroneCfg droneCfg = link_.getDroneCfg();
    dlink::Telemetry telemetry = link_.getTelemetry();

    config_.startPos.x = telemetry.x;
    config_.startPos.y = telemetry.y;
    config_.altitude = telemetry.z;
    config_.initialDir = telemetry.dir;

    config_.attackSpeed = droneCfg.attackSpeed;
    config_.accelPath = droneCfg.accelerationPath;
    config_.angularSpeed = droneCfg.angularSpeed;
    config_.turnThreshold = droneCfg.turnThreshold;
    config_.simTimeStep = droneCfg.timeStep;
    config_.timeScale = droneCfg.timeScale;

    ammoParams_.mass = ammoCfg.mass;
    ammoParams_.drag = ammoCfg.drag;
    ammoParams_.lift = ammoCfg.lift;

    size_t length = strnlen(ammoCfg.name, sizeof(ammoCfg.name));
    config_.ammoName = std::string(ammoCfg.name, length);
    config_.hitRadius = ammoCfg.hitRadius;
    
}
