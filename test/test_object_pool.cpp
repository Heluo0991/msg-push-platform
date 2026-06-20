#include"src/object_pool.h"
#include<cassert>
#include<vector>
#include<iostream>
class inte{
    private:
    int val_;
    public:
    explicit inte(int val):val_(val){};
    ~inte(){};
};

int main()
{
    std::vector<inte*> tests;
    //test1 allocate//deallocate
    {
        ObjectPool<inte,8> obj;
        for(int i=0;i<8;i++)
        {
            tests.emplace_back(obj.allocate(i));
            assert(tests[i]!=nullptr);//证明分配成功
        }
        for(int i=0;i<3;i++)
        {
            obj.deallocate(tests[i]);
        }
        for(int i=0;i<3;i++)
        {
            assert(obj.allocate(i)==tests[2-i]);//证明用的是同一片内存，不需要重新分配
        }//这应该是个LIFO的栈
        std::cout<<"test1 pass\n";
    }
    
    //test2 destuctor
    {
        ObjectPool<inte,8>obj2;
        for(int i=0;i<5;i++)
        {
            assert(obj2.allocate(i)!=nullptr);
        }
        //自动析构结点中的实例
        std::cout<<"test2 pass\n";
    }

    std::cout<<"all test pass\n";
    return 0;
}
