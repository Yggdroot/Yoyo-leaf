_Pragma("once");

#include <vector>
#include <memory>
#include "singleton.h"
#include "configReader.h"
#include "argParser.h"

namespace leaf
{

class ConfigManager : public Singleton<ConfigManager>,
    private ConfigReader, private ArgumentParser
{
    friend Singleton<ConfigManager>;
public:

    template <ConfigType T>
    decltype(auto) getConfigValue() const {
        auto idx = static_cast<uint32_t>(T);
        return static_cast<Config<typename ConfigValueType<T>::type>*>(cfg_[idx].get())->getValue();
    }

    void loadConfig(int argc, char* argv[]) {
        readConfig(cfg_);
        parseArgs(argc, argv, cfg_);
    }
private:

    ConfigManager() {
        cfg_.resize(static_cast<uint32_t>(ConfigType::MaxConfigNum));
        SetConfigValue(cfg_, Reverse, false);
        SetConfigValue(cfg_, Height, 0);
        SetConfigValue(cfg_, Indentation, 2);
    }

    std::vector<std::unique_ptr<ConfigBase>> cfg_;

};

} // end namespace leaf
