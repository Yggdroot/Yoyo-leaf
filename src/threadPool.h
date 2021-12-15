_Pragma("once");

#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include "queue.h"

namespace leaf
{

class ThreadPool
{
public:
    using Task = std::function<void()>;

    ThreadPool() = default;

    ~ThreadPool() {
        if ( running_ ) {
            stop();
        }
    }

    void start(uint32_t num) {
        if ( running_ ) {
            return;
        }

        running_ = true;
        size_ = num;
        task_queue_.initState();
        workers_.reserve(size_);
        for ( uint32_t i = 0; i < size_; ++i ) {
            workers_.emplace_back(&ThreadPool::_run, this);
        }
    }

    void stop() {
        if ( !running_ ) {
            return;
        }

        running_ = false;
        task_queue_.notifyAll();

        for (auto& t : workers_) {
            t.join();
        }

        workers_.clear();
    }

    void enqueueTask(Task&& task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            unfinished_tasks_++;
        }
        task_queue_.put(std::move(task));
    }

    void join() {
        std::unique_lock<std::mutex> lock(mutex_);
        while ( unfinished_tasks_ > 0 ) {
            task_cond_.wait(lock);
        }
    }

    size_t size() const noexcept {
        return workers_.size();
    }

private:
    void _run() {
        while ( running_ ) {
            auto task = task_queue_.take();
            if ( task ) {
                task();
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                unfinished_tasks_--;
                if ( unfinished_tasks_ == 0 ) {
                    task_cond_.notify_all();
                }
            }
        }
    }

private:
    uint32_t size_{ 0 };
    std::atomic<bool> running_{ false };
    std::vector<std::thread> workers_;
    BlockingQueue<Task>  task_queue_;
    uint32_t unfinished_tasks_{ 0 };
    std::mutex mutex_;
    std::condition_variable task_cond_;

};

} // end namespace leaf
