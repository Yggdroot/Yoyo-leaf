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


    void setBorderCharWidth(uint32_t width) {
        border_char_width_ = width;
    }

    uint32_t getBorderCharWidth() const noexcept {
        return border_char_width_;
    }

private:

    ConfigManager() {
        cfg_.resize(static_cast<uint32_t>(ConfigType::MaxConfigNum));
        SetConfigValue(cfg_, Reverse, false);
        SetConfigValue(cfg_, Height, 0);
        SetConfigValue(cfg_, Indentation, 2);
        SetConfigValue(cfg_, SortPreference, Preference::End);
        SetConfigValue(cfg_, Border, "");
        SetConfigValue(cfg_, BorderChars,
                       std::vector<std::string>({"─","│","─","│","╭","╮","╯","╰"}));
        SetConfigValue(cfg_, Margin, std::vector<uint32_t>({0, 0, 0, 0}));
    }

    std::vector<std::unique_ptr<ConfigBase>> cfg_;
    uint32_t border_char_width_{ 0 };

};

} // end namespace leaf
