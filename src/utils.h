_Pragma("once");

#include <cstdio>
#include <cstdint>
#include <array>
#include <string>

namespace utils
{
    template<uint32_t N=1024, typename... Args>
    std::string strFormat(const char* format, Args... args)
    {
        char buffer[N];
        snprintf(buffer, sizeof(buffer), format, args...);
        return std::string(buffer);
    }

    template <typename T1, typename T2>
    // return true if str1 ends with str2
    bool endswith(T1&& str1, T2&& str2) {
        static_assert(std::is_same<std::decay_t<T1>, std::string>::value
                      && std::is_same<std::decay_t<T2>, std::string>::value, "type must be std::string!");
        return str1.length() >= str2.length()
            && str1.compare(str1.length() - str2.length(), str2.length(), str2) == 0;
    }

    bool is_utf8_boundary(uint8_t c);
}
