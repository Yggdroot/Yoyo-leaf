_Pragma("once");

#define NDEBUG

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <utility>
#include <type_traits>
#include <initializer_list>

#ifdef TEST_RINGBUFFER
#include <iostream>
#endif

namespace leaf
{

#if defined(__GNUC__)

    // if x is 1, result is undefined
    #define roundup_pow_of_two(x) (1 << ((uint32_t)((sizeof(unsigned int) << 3) - __builtin_clz(x-1))))

#elif defined(__clang__)

    #if __has_builtin(__builtin_clz)
        #define roundup_pow_of_two(x) (1 << ((uint32_t)((sizeof(unsigned int) << 3) - __builtin_clz(x-1))))
    #endif

#endif

#if !defined(roundup_pow_of_two)
uint32_t roundup_pow_of_two(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
#endif

#ifdef TEST_RINGBUFFER
#define TestRingBufferLog(x)            \
    do {                                \
        std::cout << x << std::endl;    \
    } while ( 0 )

#else
#define TestRingBufferLog(x)
#endif

template<typename T>
class RingBuffer;


template <typename T, typename Container=RingBuffer<T>>
class RingBufferIterator final
{
public:
    using self = RingBufferIterator<T, Container>;
    using reference = std::conditional_t<std::is_const<Container>::value, const T&, T&>;
    using pointer = std::conditional_t<std::is_const<Container>::value, const T*, T*>;
    using size_type = size_t;

    RingBufferIterator() = default;

    explicit RingBufferIterator(Container* buffer, size_type cur)
        : ring_buffer_(buffer), current_(cur) {}


    bool operator==(const self& x) const noexcept {
        return current_ == x.current_;
    }

    bool operator!=(const self& x) const noexcept {
        return current_ != x.current_;
    }

    bool operator==(std::nullptr_t) const noexcept {
        return ring_buffer_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const noexcept {
        return ring_buffer_ != nullptr;
    }

    bool operator>(const self& x) const noexcept {
        return current_ > x.current_;
    }

    bool operator>=(const self& x) const noexcept {
        return current_ >= x.current_;
    }

    bool operator<(const self& x) const noexcept {
        return current_ < x.current_;
    }

    bool operator<=(const self& x) const noexcept {
        return current_ <= x.current_;
    }

    size_type operator-(const self& x) const noexcept {
        return (current_ + ring_buffer_->capacity_ - x.current_ ) & (ring_buffer_->capacity_ - 1);
    }

    reference operator*() const noexcept {
        return *(ring_buffer_->buffer_ + current_);
    }

    pointer operator->() const noexcept {
        return ring_buffer_->buffer_ + current_;
    }

    self& operator++() noexcept {
        current_ = (current_ + 1) & (ring_buffer_->capacity_ - 1);
        return *this;
    }

    self operator++(int) noexcept {
        self tmp = *this;
        ++*this;
        return tmp;
    }

    self operator+(size_type n) const noexcept {
        return self(ring_buffer_, (current_ + n) & (ring_buffer_->capacity_ - 1));
    }

    self operator-(size_type n) const noexcept {
        n = n & (ring_buffer_->capacity_ - 1);
        return self(ring_buffer_, (current_ + ring_buffer_->capacity_ - n) & (ring_buffer_->capacity_ - 1));
    }

    size_type current() const noexcept {
        return current_;
    }

    const T* buffer() const noexcept {
        return ring_buffer_->buffer_;
    }

    pointer data() const noexcept {
        return ring_buffer_->buffer_ + current_;
    }

    size_type capacity() const noexcept {
        return ring_buffer_->capacity_;
    }

private:
    Container* ring_buffer_ { nullptr };
    size_type current_;
};


template <typename T>
class RingBuffer
{
public:
    using iterator = RingBufferIterator<T>;
    using const_iterator = RingBufferIterator<T, const RingBuffer<T>>;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;

    friend iterator;
    friend const_iterator;

    explicit RingBuffer(size_type count=0) {
        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
                      "Must be POD!");
        if ( count > 0 ) {
            capacity_ = roundup_pow_of_two(count + 1);
            tail_ = count;
            buffer_ = new T[capacity_];
        }
    }

    RingBuffer(std::initializer_list<T> il) {
        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
                      "Must be POD!");
        if ( il.size() > 0 ) {
            capacity_ = roundup_pow_of_two(il.size() + 1);
            buffer_ = new T[capacity_];
            for (auto& x : il) {
                buffer_[tail_++] = x;
            }
        }
    }

    RingBuffer(const RingBuffer& other)
        : capacity_(other.capacity_), head_(other.head_),
        tail_(other.tail_) {
        if ( other.buffer_ == nullptr ) {
            return;
        }
        buffer_ = new T[capacity_];
        if ( tail_ > head_ ) {
            memcpy(buffer_ + head_, other.buffer_ + head_, sizeof(T) * (tail_ - head_));
        }
        else if ( tail_ < head_ ) {
            memcpy(buffer_ + head_, other.buffer_ + head_, sizeof(T) * (capacity_ - head_));
            memcpy(buffer_, other.buffer_, sizeof(T) * tail_);
        }
    }

    RingBuffer(RingBuffer&& other) noexcept
        : RingBuffer() {
        this->swap(other);
    }

    RingBuffer& operator=(RingBuffer other) noexcept {
        this->swap(other);
        return *this;
    }

    ~RingBuffer() {
        if ( buffer_ != nullptr ) {
            delete [] buffer_;
        }
    }

    size_t size() const noexcept {
        return (tail_ - head_ + capacity_) & (capacity_ - 1);
    }

    bool empty() const noexcept {
        return head_ == tail_;
    }

    void clear() noexcept {
        //if ( buffer_ != nullptr ) {
        //    delete [] buffer_;
        //    buffer_ = nullptr;
        //}
        //capacity_ = 0;
        head_ = 0;
        tail_ = 0;
    }

    void reserve(size_type n) {
        if ( n <= capacity_ ) {
            return;
        }

        auto new_capacity = roundup_pow_of_two(n + 1);
        auto tmp = buffer_;
        buffer_ = new T[new_capacity];
        if ( tail_ >= head_ ) {
            memcpy(buffer_, tmp + head_, sizeof(T) * (tail_ - head_));
        }
        else {
            auto n = capacity_ - head_;
            memcpy(buffer_ + (new_capacity - n), tmp + head_, sizeof(T) * n);
            memcpy(buffer_, tmp, sizeof(T) * tail_);
            head_ = new_capacity - n;
        }
        capacity_ = new_capacity;
        delete [] tmp;
    }

    iterator begin() noexcept {
        return iterator(this, head_);
    }

    const_iterator begin() const noexcept {
        return const_iterator(this, head_);
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(this, head_);
    }

    iterator end() noexcept {
        return iterator(this, tail_);
    }

    const_iterator end() const noexcept {
        return const_iterator(this, tail_);
    }

    const_iterator cend() const noexcept {
        return const_iterator(this, tail_);
    }

    reference operator[](size_type n) {
        assert(n >= head_ && n < tail_ || (head_ > tail_
                                           && (n >= head_ && n < capacity_ || n < tail_)));
        return buffer_[n];
    }

    const_reference operator[]( size_type n ) const {
        assert(n >= head_ && n < tail_ || (head_ > tail_
                                           && (n >= head_ && n < capacity_ || n < tail_)));
        return buffer_[n];
    }

    void push_front(const T& data) {
        _expand_space();
        head_ = (head_ + capacity_ - 1) & (capacity_ - 1);
        buffer_[head_] = data;
    }

    void push_front(T&& data) {
        _expand_space();
        head_ = (head_ + capacity_ - 1) & (capacity_ - 1);
        buffer_[head_] = std::move(data);
    }

    void push_back(const T& data) {
        _expand_space();
        buffer_[tail_] = data;
        tail_ = (tail_ + 1) & (capacity_ - 1);
    }

    void push_back(T&& data) {
        _expand_space();
        buffer_[tail_] = std::move(data);
        tail_ = (tail_ + 1) & (capacity_ - 1);
    }

    template<typename InputIterator>
    void push_front(InputIterator first, InputIterator last) {
        if ( first == last ) {
            return;
        }
        auto len = last - first;
        auto orig_size = size();
        if ( orig_size + len >= capacity_ ) { // no enough space
            TestRingBufferLog("0000000000");
            bool is_self = first.buffer() == buffer_ ;
            auto offset = is_self ? first - InputIterator(this, head_) : 0;
            auto new_capacity = roundup_pow_of_two(orig_size + len + 1);
            auto tmp = buffer_;
            buffer_ = new T[new_capacity];
            if ( tail_ >= head_ ) {
                memcpy(buffer_, tmp + head_, sizeof(T) * (tail_ - head_));
            }
            else {
                auto n = capacity_ - head_;
                memcpy(buffer_, tmp + head_, sizeof(T) * n);
                memcpy(buffer_ + n, tmp, sizeof(T) * tail_);
            }
            capacity_ = new_capacity;
            head_ = 0;
            tail_ = orig_size;
            delete [] tmp;

            if ( is_self ) {
                first = InputIterator(this, head_) + offset;
                last = first + len;
            }
        }

        if ( tail_ > head_ ) {
            if ( last > first ) { // case 1
                /*   n                            len
                //  / \                          /    \
                // |...._________.....|    |....________......|
                //      ^        ^              ^       ^
                //      |        |              |       |
                //    head_    tail_          first    last
                */
                auto n = head_;
                if ( n >= len ) {
                    TestRingBufferLog("1111111111");
                    head_ -= len;
                    memcpy(buffer_ + head_, first.data(), sizeof(T) * len);
                }
                else {
                    TestRingBufferLog("2222222222");
                    memcpy(buffer_, (last-n).data(), sizeof(T) * n);
                    head_ = capacity_ - (len - n);
                    memcpy(buffer_ + head_, first.data(), sizeof(T) * (len - n));
                }
            }
            else {
                /*   n                     len2         len1
                //  / \                     /\          /   \
                // |...._________.....|    |__........._______|
                //      ^        ^            ^        ^
                //      |        |            |        |
                //    head_    tail_         last    first
                */
                auto n = head_;
                auto len1 = first.capacity() - first.current();
                auto len2 = last.current();
                if ( n >= len2 ) {
                    n -= len2;
                    memcpy(buffer_ + n, last.buffer(), sizeof(T) * len2);
                    head_ = n;

                    // similar as case 1
                    if ( n >= len1 ) {
                        TestRingBufferLog("3333333333");
                        head_ -= len1;
                        memcpy(buffer_ + head_, first.data(), sizeof(T) * len1);
                    }
                    else {
                        TestRingBufferLog("4444444444");
                        auto k = len1 - n;
                        memcpy(buffer_, (first + k).data(), sizeof(T) * n);
                        head_ = capacity_ - k;
                        memcpy(buffer_ + head_, first.data(), sizeof(T) * k);
                    }
                }
                else {
                    TestRingBufferLog("5555555555");
                    memcpy(buffer_, last.buffer() + (len2 - n), sizeof(T) * n);
                    head_ = capacity_ - (len2 - n);
                    memcpy(buffer_ + head_, last.buffer(), sizeof(T) * (len2 - n));
                    head_ -= len1;
                    memcpy(buffer_ + head_, first.data(), sizeof(T) * len1);
                }
            }
        }
        else if ( tail_ < head_ ) {
            if ( last > first ) {
                TestRingBufferLog("6666666666");
                /*                n             len
                //              /   \         /     \
                // |__........._______|   |.._________.......|
                //    ^        ^             ^        ^
                //    |        |             |        |
                //  tail_    head_         first     last
                */
                head_ -= len;
                memcpy(buffer_ + head_, first.data(), sizeof(T) * len);
            }
            else {
                TestRingBufferLog("7777777777");
                /*                n       len2         len1
                //              /   \      /\          /   \
                // |__........._______|   |__........._______|
                //    ^        ^             ^        ^
                //    |        |             |        |
                //  tail_    head_          last    first
                */
                auto len2 = last.current();
                head_ -= len2;
                memcpy(buffer_ + head_, last.buffer(), sizeof(T) * len2);
                auto len1 = first.capacity() - first.current();
                head_ -= len1;
                memcpy(buffer_ + head_, first.data(), sizeof(T) * len1);
            }
        }
        else { // empty
            if ( last > first ) {
                TestRingBufferLog("8888888888");
                memcpy(buffer_, first.data(), sizeof(T) * len);
            }
            else {
                TestRingBufferLog("9999999999");
                auto len1 = first.capacity() - first.current();
                memcpy(buffer_, first.data(), sizeof(T) * len1);
                memcpy(buffer_ + len1, last.buffer(), sizeof(T) * last.current());
            }
            head_ = 0;
            tail_ = len;
        }
    }

    template<typename InputIterator>
    void push_back(InputIterator first, InputIterator last) {
        using namespace std;
        if ( first == last ) {
            return;
        }
        auto len = last - first;
        auto orig_size = size();
        if ( orig_size + len >= capacity_ ) { // no enough space
            TestRingBufferLog("0000000000");
            bool is_self = first.buffer() == buffer_ ;
            auto offset = is_self ? first - InputIterator(this, head_) : 0;
            auto new_capacity = roundup_pow_of_two(orig_size + len + 1);
            auto tmp = buffer_;
            buffer_ = new T[new_capacity];
            if ( tail_ >= head_ ) {
                memcpy(buffer_, tmp + head_, sizeof(T) * (tail_ - head_));
            }
            else {
                auto n = capacity_ - head_;
                memcpy(buffer_, tmp + head_, sizeof(T) * n);
                memcpy(buffer_ + n, tmp, sizeof(T) * tail_);
            }
            capacity_ = new_capacity;
            head_ = 0;
            tail_ = orig_size;
            delete [] tmp;

            if ( is_self ) {
                first = InputIterator(this, head_) + offset;
                last = first + len;
            }
        }

        if ( tail_ > head_ ) {
            if ( last > first ) { // case 1
                /*                n              len
                //              /   \          /     \
                // |.._________.......|    |.._________.......|
                //    ^        ^              ^        ^
                //    |        |              |        |
                //  head_    tail_          first     last
                */
                auto n = capacity_ - tail_;
                if ( n >= len ) {
                    TestRingBufferLog("1111111111");
                    memcpy(buffer_ + tail_, first.data(), sizeof(T) * len);
                    tail_ = (tail_ + len) & (capacity_ - 1);
                }
                else {
                    TestRingBufferLog("2222222222");
                    memcpy(buffer_ + tail_, first.data(), sizeof(T) * n);
                    tail_ = len - n;
                    memcpy(buffer_, (first + n).data(), sizeof(T) * tail_);
                }
            }
            else {
                /*                n        len2         len1
                //              /   \       /\          /   \
                // |.._________.......|    |__........._______|
                //    ^        ^              ^        ^
                //    |        |              |        |
                //  head_    tail_           last    first
                */
                auto n = capacity_ - tail_;
                auto len1 = first.capacity() - first.current();
                auto len2 = last.current();
                if ( n >= len1 ) {
                    memcpy(buffer_ + tail_, first.data(), sizeof(T) * len1);
                    tail_ += len1;
                    n = capacity_ - tail_;

                    // same as case 1
                    if ( n >= len2 ) {
                        TestRingBufferLog("3333333333");
                        memcpy(buffer_ + tail_, last.buffer(), sizeof(T) * len2);
                        tail_ = (tail_ + len2) & (capacity_ - 1);
                    }
                    else {
                        TestRingBufferLog("4444444444");
                        memcpy(buffer_ + tail_, last.buffer(), sizeof(T) * n);
                        tail_ = len2 - n;
                        memcpy(buffer_, last.buffer() + n, sizeof(T) * tail_);
                    }
                }
                else {
                    TestRingBufferLog("5555555555");
                    memcpy(buffer_ + tail_, first.data(), sizeof(T) * n);
                    tail_ = len1 - n;
                    memcpy(buffer_, (first + n).data(), sizeof(T) * tail_);
                    memcpy(buffer_ + tail_, last.buffer(), sizeof(T) * len2);
                    tail_ += len2;
                }
            }
        }
        else if ( tail_ < head_ ) {
            if ( last > first ) {
                TestRingBufferLog("6666666666");
                /*                n             len
                //              /   \         /     \
                // |__........._______|   |.._________.......|
                //    ^        ^             ^        ^
                //    |        |             |        |
                //  tail_    head_         first     last
                */
                memcpy(buffer_ + tail_, first.data(), sizeof(T) * len);
                tail_ += len;
            }
            else {
                TestRingBufferLog("7777777777");
                /*                n       len2         len1
                //              /   \      /\          /   \
                // |__........._______|   |__........._______|
                //    ^        ^             ^        ^
                //    |        |             |        |
                //  tail_    head_          last    first
                */
                auto len1 = first.capacity() - first.current();
                memcpy(buffer_ + tail_, first.data(), sizeof(T) * len1);
                tail_ += len1;
                auto len2 = last.current();
                memcpy(buffer_ + tail_, last.buffer(), sizeof(T) * len2);
                tail_ += len2;
            }
        }
        else { // empty
            if ( last > first ) {
                TestRingBufferLog("8888888888");
                memcpy(buffer_, first.data(), sizeof(T) * len);
            }
            else {
                TestRingBufferLog("9999999999");
                auto len1 = first.capacity() - first.current();
                memcpy(buffer_, first.data(), sizeof(T) * len1);
                memcpy(buffer_ + len1, last.buffer(), sizeof(T) * last.current());
            }
            head_ = 0;
            tail_ = len;
        }
    }

    /**
     * Removes the first count elements of the container.
     */
    void pop_front(size_type count = 1) {
        if ( count < size() ) {
            head_ = (head_ + count) & (capacity_ - 1);
        }
        else {
            head_ = tail_ = 0;
        }
    }

    /**
     * Removes the last count elements of the container.
     */
    void pop_back(size_type count = 1) {
        if ( count < size() ) {
            tail_ = (tail_ + capacity_ - count) & (capacity_ - 1);
        }
        else {
            head_ = tail_ = 0;
        }
    }

    T* buffer() noexcept {
        return buffer_;
    }

    size_type capacity() const noexcept {
        return capacity_;
    }

    size_type head() const noexcept {
        return head_;
    }

    size_type tail() const noexcept {
        return tail_;
    }

    void swap(RingBuffer& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
    }

#ifdef TEST_RINGBUFFER
    RingBuffer(size_type capacity, size_type head, size_type tail): head_(head), tail_(tail) {
        static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
                      "Must be POD!");
        capacity_ = roundup_pow_of_two(capacity);
        buffer_ = new T[capacity_]();
    }

#endif

private:
    void _expand_space() {
        if ( size() + 1 < capacity_ ) {
            return;
        }
        auto new_capacity = capacity_ == 0 ? 4 : roundup_pow_of_two(capacity_ + 2);
        auto tmp = buffer_;
        buffer_ = new T[new_capacity];
        if ( tail_ >= head_ ) {
            memcpy(buffer_, tmp + head_, sizeof(T) * (tail_ - head_));
        }
        else {
            auto n = capacity_ - head_;
            memcpy(buffer_, tmp + head_, sizeof(T) * n);
            memcpy(buffer_ + n, tmp, sizeof(T) * tail_);
        }
        head_ = 0;
        tail_ = capacity_ == 0 ? 0 : capacity_ - 1;
        capacity_ = new_capacity;
        delete [] tmp;
    }

private:
    T* buffer_{ nullptr };
    size_type capacity_{ 0 };
    size_type head_{ 0 };
    size_type tail_{ 0 };

};

} // end namespace leaf
