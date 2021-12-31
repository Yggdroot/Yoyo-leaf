_Pragma("once");

#include <mutex>
#include <condition_variable>
#include <deque>

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

} // end namespace leaf
