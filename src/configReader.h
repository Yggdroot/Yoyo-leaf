_Pragma("once");

#include <vector>
#include <memory>
#include "config.h"

namespace leaf
{

class ConfigReader
{
public:
    void readConfig(std::vector<std::unique_ptr<ConfigBase>>& cfg);

private:

};


} // end namespace leaf
