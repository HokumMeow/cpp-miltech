#pragma once
#include "interfaces/IConfigLoader.h"

class FileConfigLoader : public IConfigLoader {
    
public:
    FileConfigLoader(const char* path) : path_(path) {};
    void load() override;
    DroneConfig getConfig() override { return config_; }
    AmmoParams getAmmoParams() override { return ammoParams_; }
    bool isLoaded() const { return loaded_; }
    ~FileConfigLoader();
private:
    DroneConfig config_;
    AmmoParams ammoParams_;
    const char* path_;
    bool        loaded_ = false;
};