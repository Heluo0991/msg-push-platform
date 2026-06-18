#include"src/lockfree_queue.h"
#include<cassert>
#include<thread>
#include<string>
#include<iostream>
int main()
{
   
    //test1 push/pop single thread
    {
        LOCKFREEQUEUE<int,64> q1;
        assert(q1.empty());
        for(int i=0;i<10;i++)
        {
            assert(q1.push(std::move(i)));
        }
        for(int i=0;i<10;i++)
        {
            int val;
            assert(q1.pop(val));//写进去
            assert(val==i);//按FIFO是正确顺序
        }
        std::cout<<"test1 pass\n";
    }

    //test2 queue full
    {
        LOCKFREEQUEUE<int,4> q2;
        assert(q2.empty());
        for(int i=0;i<4;i++)
        {
            assert(q2.push(std::move(i)));
        }
        assert(!q2.push(5));//第五个会返回false,队列满了
        std::cout<<"test2 pass\n";
    }

    //test 3 muti thread test
    {
        LOCKFREEQUEUE<int,1024> q3;
        const int N=1000;

        //producer thread
        std::thread producer([&q3,N]{
            for(int i=0;i<N;i++)
            {
                assert(q3.push(std::move(i)));
            }
        });

        //consumer thread
        std::thread consumer([&q3,N]{
            bool seen[N]={false};
            int cnt=0;
            while(cnt<N)
            {
                int val;
                if(q3.pop(val)){
                    assert(val>=0&&val<N);//不越界
                    assert(val==cnt);//值顺序对
                    assert(!seen[val]);//不重复
                    seen[val]=true;
                    cnt++;
                }
            }
            assert(cnt==N);//不缺
        });
        producer.join();
        consumer.join();
        std::cout<<"test3 pass\n";
    }

    //test4 move test
    {
        LOCKFREEQUEUE<std::string,64> q4;
        {
            std::string s="Hello";
            assert(q4.push(std::move(s)));
            assert(s.empty());//成功移动资源
        }

        {
            std::string r;
            assert(q4.pop(r));//接收资源
            assert(r=="Hello");//无误
        }
        std::cout<<"test4 pass\n";
    }


    //test5 destructor
    {
        LOCKFREEQUEUE<int,8> q5;
        for(int i=0;i<5;i++)
        {
            assert(q5.push(std::move(i)));
        }//push五个不pop
        std::cout<<"test5 pass\n";
    }
    std::cout<<"all tests pass\n";
    return 0;
}