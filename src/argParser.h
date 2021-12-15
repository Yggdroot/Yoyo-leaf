_Pragma("once");

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
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
    std::unordered_map<std::string, std::string> alias_;
    std::unordered_map<ArgCategory, std::string> category_name_;
    std::unordered_map<std::string, Argument> args_;

};

} // end namespace leaf
