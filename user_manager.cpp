#include "user_manager.h"
#include <iostream>

std::once_flag UserManager::initialized_;
std::unique_ptr<UserManager> UserManager::instance_;

UserManager& UserManager::getInstance() {
    std::call_once(initialized_, []() {
        instance_ = std::unique_ptr<UserManager>(new UserManager());
    });
    return *instance_;
}

UserManager::UserManager() 
    : db_connection_(DatabaseManager::getInstance().getConnection()),
      insert_stmt_(nullptr), select_all_stmt_(nullptr), 
      select_by_id_stmt_(nullptr), select_by_username_stmt_(nullptr),
      update_stmt_(nullptr), delete_stmt_(nullptr) {
    prepareStatements();
}

UserManager::~UserManager() {
    finalizeStatements();
}

void UserManager::prepareStatements() {
    auto db = db_connection_.get();
    
    // 准备插入语句
    const char* insert_sql = "INSERT INTO users (username, email) VALUES (?, ?)";
    sqlite3_prepare_v2(db, insert_sql, -1, &insert_stmt_, nullptr);
    
    // 准备查询所有用户语句
    const char* select_all_sql = "SELECT id, username, email, created_at, updated_at FROM users ORDER BY id";
    sqlite3_prepare_v2(db, select_all_sql, -1, &select_all_stmt_, nullptr);
    
    // 准备根据ID查询语句
    const char* select_by_id_sql = "SELECT id, username, email, created_at, updated_at FROM users WHERE id = ?";
    sqlite3_prepare_v2(db, select_by_id_sql, -1, &select_by_id_stmt_, nullptr);
    
    // 准备根据用户名查询语句
    const char* select_by_username_sql = "SELECT id, username, email, created_at, updated_at FROM users WHERE username = ?";
    sqlite3_prepare_v2(db, select_by_username_sql, -1, &select_by_username_stmt_, nullptr);
    
    // 准备更新语句
    const char* update_sql = "UPDATE users SET username = ?, email = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_prepare_v2(db, update_sql, -1, &update_stmt_, nullptr);
    
    // 准备删除语句
    const char* delete_sql = "DELETE FROM users WHERE id = ?";
    sqlite3_prepare_v2(db, delete_sql, -1, &delete_stmt_, nullptr);
}

void UserManager::finalizeStatements() {
    if (insert_stmt_) sqlite3_finalize(insert_stmt_);
    if (select_all_stmt_) sqlite3_finalize(select_all_stmt_);
    if (select_by_id_stmt_) sqlite3_finalize(select_by_id_stmt_);
    if (select_by_username_stmt_) sqlite3_finalize(select_by_username_stmt_);
    if (update_stmt_) sqlite3_finalize(update_stmt_);
    if (delete_stmt_) sqlite3_finalize(delete_stmt_);
}

bool UserManager::createUser(const std::string& username, const std::string& email) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(insert_stmt_);
    sqlite3_bind_text(insert_stmt_, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt_, 2, email.c_str(), -1, SQLITE_STATIC);
    
    int result = sqlite3_step(insert_stmt_);
    return result == SQLITE_DONE;
}

std::vector<User> UserManager::getAllUsers() {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    std::vector<User> users;
    
    sqlite3_reset(select_all_stmt_);
    
    while (sqlite3_step(select_all_stmt_) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(select_all_stmt_, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 2));
        user.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 3));
        user.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 4));
        users.push_back(user);
    }
    
    return users;
}

User UserManager::getUserById(int id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    User user = {0, "", "", "", ""};
    
    sqlite3_reset(select_by_id_stmt_);
    sqlite3_bind_int(select_by_id_stmt_, 1, id);
    
    if (sqlite3_step(select_by_id_stmt_) == SQLITE_ROW) {
        user.id = sqlite3_column_int(select_by_id_stmt_, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 2));
        user.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 3));
        user.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 4));
    }
    
    return user;
}

User UserManager::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    User user = {0, "", "", "", ""};
    
    sqlite3_reset(select_by_username_stmt_);
    sqlite3_bind_text(select_by_username_stmt_, 1, username.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(select_by_username_stmt_) == SQLITE_ROW) {
        user.id = sqlite3_column_int(select_by_username_stmt_, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(select_by_username_stmt_, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(select_by_username_stmt_, 2));
        user.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_username_stmt_, 3));
        user.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_username_stmt_, 4));
    }
    
    return user;
}

bool UserManager::updateUser(int id, const std::string& username, const std::string& email) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(update_stmt_);
    sqlite3_bind_text(update_stmt_, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(update_stmt_, 2, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(update_stmt_, 3, id);
    
    int result = sqlite3_step(update_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool UserManager::deleteUser(int id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(delete_stmt_);
    sqlite3_bind_int(delete_stmt_, 1, id);
    
    int result = sqlite3_step(delete_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool UserManager::createUsersTransaction(const std::vector<std::pair<std::string, std::string>>& users) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    auto db = db_connection_.get();
    
    // 开始事务
    char* error_msg = nullptr;
    int result = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        sqlite3_free(error_msg);
        return false;
    }
    
    // 批量插入用户
    for (const auto& user_pair : users) {
        sqlite3_reset(insert_stmt_);
        sqlite3_bind_text(insert_stmt_, 1, user_pair.first.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt_, 2, user_pair.second.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(insert_stmt_) != SQLITE_DONE) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
    }
    
    // 提交事务
    result = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        sqlite3_free(error_msg);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    
    return true;
}
