#include <fstream>
#include <cstring>
#include <iostream>

#include "core/FileConfigLoader.h"
#include "core/Log.h"

#include "json.hpp"

using json = nlohmann::json;

void FileConfigLoader::load(){

    std::ifstream fin(path_ + std::string("/config.json"));
    if (!fin.is_open())
    {
        std::cerr << "Error opening config file" << std::endl;
        return;
    }

    json j;
    fin >> j;

    config_.startPos.x = j["drone"]["position"]["x"];
    config_.startPos.y = j["drone"]["position"]["y"];
    config_.altitude = j["drone"]["altitude"];
    config_.initialDir = j["drone"]["initialDirection"];
    config_.attackSpeed = j["drone"]["attackSpeed"];
    config_.accelPath = j["drone"]["accelerationPath"];
    config_.angularSpeed = j["drone"]["angularSpeed"];
    config_.turnThreshold = j["drone"]["turnThreshold"];
    config_.arrayTimeStep = j["targetArrayTimeStep"];
    config_.simTimeStep   = j["simulation"]["timeStep"];
    config_.hitRadius     = j["simulation"]["hitRadius"];

    std::string ammoStr = j["ammo"].get<std::string>();
    std::strncpy(config_.ammoName, ammoStr.c_str(), 31);

    LOG("Config loaded: x=" << config_.startPos.x);
    LOG("Config loaded: y=" << config_.startPos.y);
    LOG("Config loaded: speed=" << config_.attackSpeed);
    LOG("Config loaded: altitude=" << config_.altitude);
    LOG("Config loaded: initial direction=" << config_.initialDir);
    LOG("Config loaded: attack speed=" << config_.attackSpeed);
    LOG("Config loaded: turn threshold=" << config_.turnThreshold);
    LOG("Config loaded: array time step=" << config_.arrayTimeStep);
    LOG("Config loaded: simulation time step=" << config_.simTimeStep);
    LOG("Config loaded: hit radius=" << config_.hitRadius);

    ////////////////////////////////////////////////////////////////////////
    // Читання JSON ammo
    std::ifstream f_a(path_ + std::string("/ammo.json"));
    if (!f_a.is_open())
    {
        std::cerr << "Error opening ammo file" << std::endl;
        return;
    }

    json j_a;
    f_a >> j_a;
    int ammoCount = j_a.size();
    int selectedAmmo = -1;
    for (int i = 0; i < ammoCount; i++) {
        AmmoParams current;
        std::strncpy(current.name, j_a[i]["name"].get<std::string>().c_str(), 31);
        current.mass = j_a[i]["mass"];
        current.drag = j_a[i]["drag"];
        current.lift = j_a[i]["lift"];
        if (strcmp(current.name, config_.ammoName) == 0) {
            ammoParams_ = current; 
            selectedAmmo = i;
        }
    }
    if (selectedAmmo == -1)
    {
        std::cerr << "Unknown ammo!" << std::endl;
        return;
    }

    LOG("Config loaded: ammo=" << config_.ammoName);

    loaded_ = true;
}