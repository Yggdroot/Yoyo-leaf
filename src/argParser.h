_Pragma("once");

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "error.h"
#include "utils.h"
#include "config.h"

namespace leaf
{

enum class ArgCategory {
    Layout,
    Search,

    MaxNum
};

struct Argument
{
    ArgCategory category;
    std::string alias;
    ConfigType cfg_type;
    std::string nargs;
    std::string metavar;
    std::string help;
};

class ArgumentParser
{
public:
    ArgumentParser();
    void printHelp();
    void parseArgs(int argc, char *argv[], std::vector<std::unique_ptr<ConfigBase>>& cfg);
private:
    template<typename... Args>
    static void appendError(Args... args) {
        Error::getInstance().appendError(utils::strFormat<512>(std::forward<Args>(args)...));
    }

    template <typename T>
    static std::vector<T> parseList(const std::string& key, const std::string& str,
                                    std::function<T(const std::string&)> convert);

private:
    std::unordered_map<std::string, std::string> alias_;
    std::unordered_map<ArgCategory, std::string> category_name_;
    std::unordered_map<std::string, Argument> args_;

};

/**
 * str is a string like "[1, 2, 3, 4]"
 */
template <typename T>
std::vector<T> ArgumentParser::parseList(const std::string& key, const std::string& str,
                                         std::function<T(const std::string&)> convert) {
    auto left = str.find('[');
    auto right = str.find(']');
    if ( left == std::string::npos || right == std::string::npos ) {
        appendError("invalid value: %s for %s", str.c_str(), key.c_str());
        std::exit(EXIT_FAILURE);
    }

    std::vector<T> res;
    res.reserve(4);
    for ( auto i = left + 1; i < right; ++i ) {
        while ( str[i] == ' ' ) {
            ++i;
        }

        auto start = i;
        while ( i < right && str[i] != ' ' && str[i] != ',' ) {
            ++i;
        }
        if ( i == start ) {
            appendError("invalid value: %s for %s", str.c_str(), key.c_str());
            std::exit(EXIT_FAILURE);
        }
        res.emplace_back(convert(str.substr(start, i - start)));

        while ( str[i] == ' ' ) {
            ++i;
        }

        if ( i < right && str[i] != ',' ) {
            appendError("invalid value: %s for %s", str.c_str(), key.c_str());
            std::exit(EXIT_FAILURE);
        }
    }

    return res;
}

} // end namespace leaf
