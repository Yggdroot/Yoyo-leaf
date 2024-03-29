_Pragma("once");

#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <type_traits>

namespace leaf
{

enum class ConfigType
{
    Reverse,
    Height,
    Indentation,
    SortPreference,
    Border,
    BorderChars,
    Margin,

    MaxConfigNum
};

enum class Preference {
    Begin,
    End,
};

template <ConfigType T>
struct ConfigValueType {
    using type = bool;
};

#define DefineConfigValue(cfg_type, val_type)                               \
    template <>                                                             \
    struct ConfigValueType<ConfigType::cfg_type> {                          \
        using type = val_type;                                              \
        static_assert(std::is_same<std::decay_t<type>, type>::value, "");   \
    };

DefineConfigValue(Height, uint32_t)
DefineConfigValue(Indentation, uint32_t)
DefineConfigValue(SortPreference, Preference)
DefineConfigValue(Border, std::string)
DefineConfigValue(BorderChars, std::vector<std::string>)
DefineConfigValue(Margin, std::vector<uint32_t>)

#define SetConfigValue(container, cfg_type, value)                  \
    container[static_cast<uint32_t>(ConfigType::cfg_type)].reset(   \
        new Config<typename ConfigValueType<ConfigType::cfg_type>::type>(value))

class ConfigBase
{
public:
    virtual ~ConfigBase() = default;

};

template <typename T>
class Config : public ConfigBase
{
    static_assert(std::is_same<std::decay_t<T>, T>::value, "");
public:
    explicit Config(const T& val): value_(val) {

    }

    explicit Config(T&& val): value_(std::move(val)) {

    }

    std::conditional_t<std::is_fundamental<T>::value, T, T&> getValue() noexcept {
        return value_;
    }

private:
    T value_;

};

} // end namespace leaf
