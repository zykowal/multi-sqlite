#pragma once

#include "database_manager.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>

struct User {
    int id;
    std::string username;
    std::string email;
    std::string created_at;
    std::string updated_at;
};

class UserManager {
public:
    static UserManager& getInstance();
    ~UserManager();
    
    // 用户操作
    bool createUser(const std::string& username, const std::string& email);
    std::vector<User> getAllUsers();
    User getUserById(int id);
    User getUserByUsername(const std::string& username);
    bool updateUser(int id, const std::string& username, const std::string& email);
    bool deleteUser(int id);
    
    // 批量操作
    bool createUsersTransaction(const std::vector<std::pair<std::string, std::string>>& users);
    
private:
    UserManager();
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;
    
    // 预编译语句
    void prepareStatements();
    void finalizeStatements();
    
    std::shared_ptr<sqlite3> db_connection_;
    std::mutex operation_mutex_;
    
    // 预编译的SQL语句
    sqlite3_stmt* insert_stmt_;
    sqlite3_stmt* select_all_stmt_;
    sqlite3_stmt* select_by_id_stmt_;
    sqlite3_stmt* select_by_username_stmt_;
    sqlite3_stmt* update_stmt_;
    sqlite3_stmt* delete_stmt_;
    
    static std::once_flag initialized_;
    static std::unique_ptr<UserManager> instance_;
};
