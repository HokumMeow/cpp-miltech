#include <fstream>
#include <map>
#include <iostream>
#include "config/FileConfigLoader.h"
#include "Log.h"

#include "json.hpp"

using json = nlohmann::json;

void FileConfigLoader::load(){

    std::ifstream fin(path_ + "/config.json");
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
    config_.ammoName = ammoStr;
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
    std::ifstream f_a(path_ + "/ammo.json");
    if (!f_a.is_open())
    {
        std::cerr << "Error opening ammo file" << std::endl;
        return;
    }

    json j_a;
    f_a >> j_a;
    std::map<std::string, AmmoParams> ammoTable;
    for (auto& entry : j_a)
    {
        AmmoParams current;
        current.name = entry["name"].get<std::string>();
        current.mass = entry["mass"];
        current.drag = entry["drag"];
        current.lift = entry["lift"];
        ammoTable[current.name] = current;
    }

    auto it = ammoTable.find(config_.ammoName);
    if (it == ammoTable.end()) {
        
        std::cerr << "Unknown ammo!" << std::endl;
        return;
    }
    ammoParams_ = it->second;

    LOG("Config loaded: ammo=" << config_.ammoName);

    loaded_ = true;
}

FileConfigLoader::~FileConfigLoader() {
    
    
}