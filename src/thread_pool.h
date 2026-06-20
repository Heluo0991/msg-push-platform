#ifndef THREAD_POOL
#define THREAD_POOL
#include<mutex>
#include<condition_variable>
#include<queue>
#include<thread>
#include<vector>
#include<functional>
#include<utility>
#include<cstddef>

class ThreadPool{
private:
    using Task=std::function<void()>;
    std::vector<std::thread> thread_pool_;
    std::condition_variable cv_;
    std::queue<Task> tasks_;
    std::mutex mtx_;
    bool stop_;

public:
    explicit ThreadPool(size_t n);
    ThreadPool() = delete;
    ThreadPool(const ThreadPool&)=delete;
    ThreadPool& operator=(const ThreadPool&)=delete;
    ~ThreadPool();

    template<typename F>
    void submit(F&& f)
    {
        {   
        std::lock_guard<std::mutex> lock(mtx_);
        tasks_.emplace(std::forward<F>(f));
        }
        cv_.notify_one();//送任务，叫一个线程领任务
  
    }
   
};
#endif