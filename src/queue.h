_Pragma("once");

#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>

namespace leaf
{

template<typename T>
class BlockingQueue
{
public:
    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;
    BlockingQueue(BlockingQueue&&) = delete;
    BlockingQueue& operator=(BlockingQueue&&) = delete;

    BlockingQueue() = default;

    void put(const T& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(data);
        not_empty_.notify_one();
    }

    void put(T&& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(std::move(data));
        not_empty_.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lock(mutex_);
        while ( queue_.empty() && running_ ) {
            not_empty_.wait(lock);
        }

        // queue_ is empty only when notifyAll() is called
        if ( queue_.empty() ) {
            return T();
        }

        T front(std::move(queue_.front()));
        queue_.pop_front();
        return front;
    }

    // take care to call this function
    void notifyAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        not_empty_.notify_all();
    }

    void initState() {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    bool running_{ true };
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::deque<T> queue_;
};

template<typename T>
class RingQueue
{
public:
    RingQueue(const RingQueue&) = delete;
    RingQueue& operator=(const RingQueue&) = delete;
    RingQueue(RingQueue&&) = delete;
    RingQueue& operator=(RingQueue&&) = delete;

    explicit RingQueue(uint32_t capacity): capacity_(capacity), queue_(capacity + 1){}

    void put(const T& data) {
        queue_[tail_] = data;
        tail_ = (tail_ + 1) % capacity_;
    }

    void put(T&& data) {
        queue_[tail_] = std::move(data);
        tail_ = (tail_ + 1) % capacity_;
    }

    T take() {
        auto data = std::move(queue_[head_]);
        head_ = (head_ + 1) % capacity_;
        return data;
    }

    bool empty() {
        return head_ == tail_;
    }


private:
    uint32_t capacity_;
    uint32_t head_{ 0 };
    uint32_t tail_{ 0 };
    std::vector<T> queue_;
};

} // end namespace leaf
