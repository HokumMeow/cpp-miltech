class IConfigLoader {
public:
    virtual int    load() = 0;
    virtual int    getConfig() = 0;
    virtual int    getAmmoParams() = 0;
    virtual ~IConfigLoader() {}
};
