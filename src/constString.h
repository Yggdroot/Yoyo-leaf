_Pragma("once");

#include <cstdint>

namespace leaf
{

// POD
struct ConstString
{
    const char* str;
    uint32_t    len;
};

static inline ConstString makeConstString(const char* str, uint32_t len) {
    ConstString const_str;
    const_str.str = str;
    const_str.len = len;
    return const_str;
}

} // end namespace leaf
