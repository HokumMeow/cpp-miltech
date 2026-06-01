#include "FileConfigLoader.h"
#include <fstream>
#include <cstring>
#include <iostream>

#include "json.hpp"

using json = nlohmann::json;

FileConfigLoader::FileConfigLoader(const char* path)
{
    load();
}

void FileConfigLoader::load(){

    std::ifstream fin("config.json");
    json j;
    fin >> j;

    DroneConfig config;
    config.startPos.x = j["drone"]["position"]["x"];
    config.startPos.y = j["drone"]["position"]["y"];
    config.altitude = j["drone"]["altitude"];
    config.initialDir = j["drone"]["initialDirection"];
    config.attackSpeed = j["drone"]["attackSpeed"];
    config.accelPath = j["drone"]["accelerationPath"];
    config.angularSpeed = j["drone"]["angularSpeed"];
    config.turnThreshold = j["drone"]["turnThreshold"];
    config.arrayTimeStep = j["targetArrayTimeStep"];
    config.simTimeStep   = j["simulation"]["timeStep"];
    config.hitRadius     = j["simulation"]["hitRadius"];

    std::string ammoStr = j["ammo"].get<std::string>();
    std::strncpy(config.ammoName, ammoStr.c_str(), 31);

    LOG("Config loaded: x=" << config.startPos.x);
    LOG("Config loaded: y=" << config.startPos.y);
    LOG("Config loaded: speed=" << config.attackSpeed);
    LOG("Config loaded: altitude=" << config.altitude);
    LOG("Config loaded: initial direction=" << config.initialDir);
    LOG("Config loaded: attack speed=" << config.attackSpeed);
    LOG("Config loaded: turn threshold=" << config.turnThreshold);
    LOG("Config loaded: array time step=" << config.arrayTimeStep);
    LOG("Config loaded: simulation time step=" << config.simTimeStep);
    LOG("Config loaded: hit radius=" << config.hitRadius);

    ////////////////////////////////////////////////////////////////////////
    // Читання JSON ammo
    std::ifstream f_a("ammo.json");
    if (!f_a.is_open())
    {
        std::cerr << "Error opening ammo file" << std::endl;
        //return 1;
    }

    json j_a;
    f_a >> j_a;
    int ammoCount = j_a.size();
    int selectedAmmo = -1;
    AmmoParams* ammo = new AmmoParams[ammoCount];
    for (int i = 0; i < ammoCount; i++) {
        std::strncpy(ammo[i].name, j_a[i]["name"].get<std::string>().c_str(), 31);
        ammo[i].mass = j_a[i]["mass"];
        ammo[i].drag = j_a[i]["drag"];
        ammo[i].lift = j_a[i]["lift"];
        if (strcmp(ammo[i].name, config.ammoName) == 0)
        {
            selectedAmmo = i;
        }
    }
    if (selectedAmmo == -1)
    {
        std::cerr << "Unknown ammo!" << std::endl;
        delete[] ammo;
        return 1;
    }

    LOG("Config loaded: ammo=" << config.ammoName);
}