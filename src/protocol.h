#ifndef PROTOCOL
#define PROTOCOL
#include <string>
#include "message.h"
#include "json.hpp"

using json = nlohmann::json;

void try_parse_line(const std::string &raw, MessageBody &out)
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
        out = GroupMsg{js["from"], js["group_name"], js["content"]};
    }
    else if (type == "private")
    {
        out = PrivateMsg{js["from"], js["to"], js["content"]};
    }
    else if (type == "ack")
    {
        out = AckMsg{js["msg_id"]};
    }
    else
    {
        out = ErrorMsg{-1, "unknown message type: " + type};
    }
}

#endif