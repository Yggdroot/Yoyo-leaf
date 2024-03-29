_Pragma("once");

#include <type_traits>
#include <string>
#include <vector>
#include <mutex>
#include <cstdio>
#include <string.h>
#include "singleton.h"
#include "utils.h"


namespace leaf
{

#define ErrorMessage \
    utils::strFormat("%s:%d:%s", __FILE__, __LINE__, strerror(errno))

class Error final : public Singleton<Error>
{
    friend Singleton<Error>;
public:
    template <typename T,
             typename=std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value>
             >
    void appendError(T&& err) {
        std::lock_guard<std::mutex> lock(mutex_);
        err_msgs_.emplace_back(std::forward<T>(err));
    }

    ~Error() {
        for ( const auto& err : err_msgs_ ) {
            fprintf(stderr, "%s\r\n", err.c_str());
        }
    }
private:
    std::mutex mutex_;
    std::vector<std::string> err_msgs_;
};

} // end namespace leaf
