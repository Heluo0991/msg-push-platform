#ifndef PROTOCOL
#define PROTOCOL
#include <string>
#include "message.h"
#include "json.hpp"

using json = nlohmann::json;

inline void try_parse_line(const std::string &raw, MessageBody &out)//需要内联，在.h里写实现
{
    auto js = json::parse(raw);
    std::string type = js["type"]; // 键值对
    if (type == "chat")
    {
        out = ChatMsg{js["from"], js["content"]};
    }
    else if (type == "login")
    {
        out = LoginMsg{js["username"], js["password_hash"]};
    }
    else if (type == "group")
    {
        out = GroupMsg{js.value("from", ""), js.value("cmd", ""), js.value("group", ""), js.value("content", "")};
    }
    else if (type == "private")
    {
        out = PrivateMsg{js["from"], js["to"], js["content"]};
    }
    /*
    else if (type == "ack")
    {
        out = AckMsg{js["msg_id"]};
    }
    */
    else
    {
        out = ErrorMsg{-1, "unknown message type: " + type};
    }
}//把生json字符串处理成Msg结构体

#endif