#pragma once
#include "interfaces/IConfigLoader.h"
#include <link/UartLink.h>

class UartConfigLoader : public IConfigLoader {
    
public:
    UartConfigLoader(UartLink& link) : link_(link) {};
    void load() override;
    DroneConfig getConfig() override { return config_; }
    AmmoParams getAmmoParams() override { return ammoParams_; }

private:
    DroneConfig config_;
    AmmoParams ammoParams_;
    UartLink& link_;

};