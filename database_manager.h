#pragma once

#include <sqlite3.h>
#include <memory>
#include <mutex>
#include <string>
#include <stdexcept>

class DatabaseManager {
public:
    static DatabaseManager& getInstance();
    std::shared_ptr<sqlite3> getConnection();
    void initializeTables();
    ~DatabaseManager();
    
private:
    DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    void openDatabase();
    void configureDatabase();
    
    std::shared_ptr<sqlite3> db_connection_;
    std::mutex connection_mutex_;
    static std::once_flag initialized_;
    static std::unique_ptr<DatabaseManager> instance_;
    
    const std::string db_path_ = "app_database.db";
};

// 自定义删除器用于sqlite3指针
struct SQLiteDeleter {
    void operator()(sqlite3* db) {
        if (db) {
            sqlite3_close_v2(db);
        }
    }
};
