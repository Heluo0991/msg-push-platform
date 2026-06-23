#ifndef RAWMESSAGE
#define RAWMESSAGE
#include<memory>
#include"connection.h"
struct RawMessage{
    std::weak_ptr<Connection> wp_;//指向Connection的弱指针
    std::string raw_;//生字符串本身

    RawMessage(std::shared_ptr<Connection> sp,std::string raw):wp_(sp),raw_(raw){}
    //调用weak_ptr的重载
};

#endif