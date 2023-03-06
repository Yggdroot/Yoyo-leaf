_Pragma("once");

#include <utility>

namespace leaf
{

template <typename T>
class Singleton 
{
public:

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    template<typename... Args>
    static T& getInstance(Args&&... args) {
        static T t(std::forward<Args>(args)...);
        return t;
    }

protected:
    Singleton() = default;

};

} // end namespace leaf
