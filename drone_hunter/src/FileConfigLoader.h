#pragma once
#include "interfaces/IConfigLoader.h"

class FileConfigLoader : public IConfigLoader {
    
public:
    FileConfigLoader(const char* path);
    void load() override;
    DroneConfig getConfig() override { return config_; }
    AmmoParams getAmmoParams() override { return ammoParams_; }
    ~FileConfigLoader();
private:
    DroneConfig config_;
    AmmoParams ammoParams_;
};