#ifndef RAWMESSAGE
#define RAWMESSAGE
#include<memory>
#include"connection.h"
struct RawMessgae{
    std::weak_ptr<Connection> wp_;//指向Connection的弱指针
    std::string raw;//生字符串本身
};

#endif