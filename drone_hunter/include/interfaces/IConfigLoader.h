#pragma once
#include "dto/AmmoParams.h"
#include "dto/DroneConfig.h"

class IConfigLoader {
public:
    virtual void load() = 0;
    virtual DroneConfig getConfig() = 0;
    virtual AmmoParams getAmmoParams() = 0;
    virtual ~IConfigLoader() {}
};
