#include "thread_pool.h"
#include<iostream>
THREADPOOL::THREADPOOL(size_t n) : stop_(false)
{
    for (size_t i = 0; i < n; i++)
    {
        thread_pool_.emplace_back([this] {
            while(true)
            {
                Task task;
                {
                    std::unique_lock<std::mutex> lock(mtx_);//操作队列需要有锁
                    cv_.wait(lock,[this]{
                        return stop_||!tasks_.empty();
                    });//要么停机处理要么有任务领
                    if(stop_&&tasks_.empty())return;//无任务且停机，线程结束
                    task=std::move(tasks_.front());//从对头领任务
                    tasks_.pop();
                }
                task();//有任务就可以干活
            }//持续执行的线程

        });
    }
}

THREADPOOL::~THREADPOOL()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        stop_=true;//停机
    }
    cv_.notify_all();//全部唤醒处理剩余任务
    for(auto &t :thread_pool_)t.join();//等待所有线程执行完毕
}