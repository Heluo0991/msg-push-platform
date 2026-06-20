#include "src/thread_pool.h"
#include <cassert>
#include <atomic>
#include <iostream>
#include <mutex>
#include <set>
#include <chrono>

int main()
{
    // test1 task finish
    {
        std::atomic<int> x{0};
        {
            ThreadPool pool(2);
            for (int i = 0; i < 100; i++)
                pool.submit([&x]
                            { x.fetch_add(1); });
        }
        assert(x == 100);
        std::cout << "test1 pass\n";
    }

    // test2 parrall
    {
        ThreadPool pool(10);
        std::mutex mtx_;
        std::atomic<int> done_num = 0;
        std::set<std::thread::id> ids; // 非重有序集合

        for (int i = 0; i < 10; i++)
        {
            pool.submit([&done_num, &mtx_, &ids]
                        {
                            {
                                std::lock_guard<std::mutex> lock(mtx_);
                                ids.insert(std::this_thread::get_id());
                            }
                            done_num.fetch_add(1); // 累加不需要上锁，原子锁
                        });
        }

        while (done_num < 10)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // 等待所有线程完成
        assert(done_num == 10);   // 都完成了
        assert(ids.size() > 1);   // 至少两个线程在完成这个任务
        assert(ids.size() <= 10); // 没有超过线程池大小

        std::cout << "test2 pass\n";
    }

    // test3 auto destructor
    {
        std::atomic<int> counter{0};
        {
            ThreadPool pool(4);

            for (int i = 0; i < 100; i++)
            {
                pool.submit(
                    [&counter]
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 模拟pool实例析构了还没执行完
                        counter.fetch_add(1);
                    });
            }
        }
        // pool 析构时会将剩余任务执行完再退出
        assert(counter == 100);
        std::cout << "test3 pass\n";
    }
    return 0;
}