#include "db_store.h"
#include <cassert>
#include <iostream>
int main()
{

    DBstore db("./test.db");
    // test1 注册用户
    {
        assert(db.register_user("Alice", "1234"));
        assert(!db.register_user("Alice", "4321"));
        // 唯一主键，一次注册成功二次注册失败
        std::cout << "test1 pass\n";
    }

    // test2 登录
    {
        assert(db.verify_login("Alice", "1234"));
        assert(!db.verify_login("Alice", "4321"));
        // 登录成功和密码错误
        std::cout << "test2 pass\n";
    }

    // test3 离线信息存储
    {
        assert(db.register_user("Bob", "1111"));
        assert(db.store_offline("Bob", "test message1"));
        assert(db.store_offline("Bob", "test message2"));
        assert(db.store_offline("Bob", "test message3"));
        // 存三个离线信息
        std::cout << "test3 pass\n";
    }

    {
        
        std::vector<std::pair<std::string, std::string>> fetch_msgs;
        // test4 拉取离线信息
        {
            fetch_msgs = std::move(db.fetch_offline("Bob"));
            assert(fetch_msgs.size() == 3); // 拉了三条
            for (auto it : fetch_msgs)
            {
                std::cout << "msg_id : " << it.first << " msg : " << it.second << '\n';
            }
            std::cout << "test4 pass\n";
        }

        //test5 确认发送离线信息
        {
            assert(db.mark_delivered(fetch_msgs[0].first));//确认第一条已经发出
            std::cout << "test5 pass\n";
        }

        std::cout<<"all tests pass\n";
    }

    return 0 ;
}