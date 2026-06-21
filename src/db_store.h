#ifndef DB_STORE
#define DB_STORE
#include<sqlite3.h>
#include<vector>
#include"message.h"
#include<ctime>

class DBstore{
private:
    sqlite3* db_=nullptr;
public:
    DBstore(const std::string &);//根据路径建表
    DBstore(const DBstore& )=delete;
    DBstore& operator=(const DBstore&)=delete;
    DBstore(DBstore&&)=delete;
    DBstore& operator=(DBstore&&)=delete;
    bool register_user(const std::string&, const std::string&);//注册
    bool verify_login(const std::string& ,const std::string&) ;//验证哈希值登录
    std::vector<std::pair<std::string,std::string>> fetch_offline(const std::string&) ;//拉取离线信息
    bool store_offline( const std::string& ,const std::string&);//存储离线信息
    bool mark_delivered(const std::string&);//删除已验证信息
    ~DBstore();
};

#endif