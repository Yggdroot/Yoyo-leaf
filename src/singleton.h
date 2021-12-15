_Pragma("once");

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

    static T& getInstance() {
        static T t;
        return t;
    }

protected:
    Singleton() = default;

};

} // end namespace leaf
