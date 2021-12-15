_Pragma("once");

#include <type_traits>
#include <string>
#include <vector>
#include <mutex>
#include <errno.h>
#include <stdio.h>
#include "singleton.h"


namespace leaf
{

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
            printf("%s\r\n", err.c_str());
        }
    }
private:
    std::mutex mutex_;
    std::vector<std::string> err_msgs_;
};

} // end namespace leaf
