#pragma once

#include "multi_connection_database_manager.h"
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

class MultiConnectionUserManager {
public:
    static MultiConnectionUserManager& getInstance();
    ~MultiConnectionUserManager();
    
    // 用户操作
    bool createUser(const std::string& username, const std::string& email);
    std::vector<User> getAllUsers();
    User getUserById(int id);
    User getUserByUsername(const std::string& username);
    bool updateUser(int id, const std::string& username, const std::string& email);
    bool deleteUser(int id);
    
    // 批量操作
    bool createUsersTransaction(const std::vector<std::pair<std::string, std::string>>& users);
    
    // 性能测试方法
    void performanceTest(int thread_count, int operations_per_thread);
    
private:
    MultiConnectionUserManager();
    MultiConnectionUserManager(const MultiConnectionUserManager&) = delete;
    MultiConnectionUserManager& operator=(const MultiConnectionUserManager&) = delete;
    
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
    static std::unique_ptr<MultiConnectionUserManager> instance_;
};
