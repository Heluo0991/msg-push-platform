#include "db_store.h"

DBstore::DBstore(const std::string &path)
{
    {
        int rc = sqlite3_open(path.c_str(), &db_); // rc=returncode
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "[DBStore] create failed\n");
            sqlite3_close(db_); // 打开数据库失败，关闭文件
        }
    } // 初始化数据库

    {
        char *err_msg = nullptr;
        int rc = sqlite3_exec(
            db_,
            "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY , password TEXT NOT NULL)",
            nullptr,
            nullptr,
            &err_msg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "[DBStore] create users failed %s\n", err_msg); // 打印错误信息
            sqlite3_free(err_msg);                                          // 释放错误信息实例
        }
    } // 建users表

    {
        char *err_msg = nullptr;
        int rc = sqlite3_exec(
            db_,
            "CREATE TABLE IF NOT EXISTS offline_msgs (msg_id TEXT PRIMARY KEY , username TEXT NOT NULL, msg_json TEXT NOT NULL, timestamp INTEGER NOT NULL)",
            nullptr,
            nullptr,
            &err_msg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "[DBStore] create offline_msg failed %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    } // 建离线信息表
}

bool DBstore::register_user(const std::string &user, const std::string &hash)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO users (username , password) VALUES ( ? , ? ) ";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        fprintf(stderr, "[DBStore] prepare failed: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "[DBStore] regsiter failed %s\n", sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool DBstore::verify_login(const std::string &user, const std::string &hash)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT password FROM users WHERE username = ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) // 编译模板
    {
        fprintf(stderr, "[DBStore] prepare failed: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *stored = sqlite3_column_text(stmt, 0); // 获得select的第0列
        std::string stored_str(reinterpret_cast<const char *>(stored));
        ok = (stored_str == hash); // 密码与查询用户密码一致
    }
    sqlite3_finalize(stmt);
    return ok;
} // 登录认证

bool DBstore::store_offline(const std::string &user, const std::string &msg_json)
{

    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO offline_msgs (msg_id , username , msg_json , timestamp) VALUES ( ? , ? , ? , ? )";

    // 维护一个静态局部变量，不会重复初始化，只在函数内可见，在.data数据段
    static int counter = 0;
    //

    std::string msg_id = user + "_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(counter++);
    // msg_id格式 "name_1111111_1"，以Unix时间戳mark，以消息计数器为counter，并且不暴露在头文件
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        fprintf(stderr, "[DBStore] prepare failed: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    sqlite3_bind_text(stmt, 1, msg_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, msg_json.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, std::time(nullptr));

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "[DBStore] store offline_msg failed %s\n", sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
} // 传入用户名和json格式信息

std::vector<std::pair<std::string, std::string>> DBstore::fetch_offline(const std::string &user)
{
    std::vector<std::pair<std::string, std::string>> msgs;
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT msg_id , msg_json  FROM offline_msgs WHERE username = ?";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        fprintf(stderr, "[DBStore] prepare failed: %s\n", sqlite3_errmsg(db_));
        return msgs; // 返回空vector
    }
    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string id(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
        std::string json(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        msgs.emplace_back(std::move(id), std::move(json)); // 直接取走，用string的移动构造
    } // 数据库本身数据不变，到时候确认有没有发出去就行，不需要报错这里
    sqlite3_finalize(stmt);
    return msgs;
} // 拉取全部离线信息，返回msg_id和字符串内容

bool DBstore::mark_delivered(const std::string &msg_id)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM offline_msgs WHERE msg_id = ? ";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        fprintf(stderr, "[DBStore] prepare failed: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    sqlite3_bind_text(stmt, 1, msg_id.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "[DBStore] mark_delivered failed %s\n", sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
} // 根据id标记已投递离线信息，删除对应信息

bool DBstore::user_exists(const std::string &user)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT COUNT(*) FROM users WHERE username = ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0);
    sqlite3_finalize(stmt);
    return exists;
}//查询用户是否存在

DBstore::~DBstore()
{
    int rc = sqlite3_close(db_);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "[DBStore] close failed %s\n", sqlite3_errmsg(db_));
    }
}