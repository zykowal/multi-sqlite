#pragma once

#include <sqlite3.h>
#include <memory>
#include <mutex>
#include <string>
#include <stdexcept>
#include <unordered_map>

// 多连接数据库管理器
class MultiConnectionDatabaseManager {
public:
    enum class TableType {
        USERS,
        ORDERS,
        PRODUCTS
    };
    
    static MultiConnectionDatabaseManager& getInstance();
    std::shared_ptr<sqlite3> getConnection(TableType table);
    void initializeAllTables();
    
    // 跨表事务支持
    bool beginDistributedTransaction();
    bool commitDistributedTransaction();
    bool rollbackDistributedTransaction();
    
    ~MultiConnectionDatabaseManager();
    
private:
    MultiConnectionDatabaseManager();
    MultiConnectionDatabaseManager(const MultiConnectionDatabaseManager&) = delete;
    MultiConnectionDatabaseManager& operator=(const MultiConnectionDatabaseManager&) = delete;
    
    void openConnection(TableType table, const std::string& db_path);
    void configureConnection(sqlite3* db);
    void initializeTable(TableType table);
    
    std::unordered_map<TableType, std::shared_ptr<sqlite3>> connections_;
    std::unordered_map<TableType, std::string> db_paths_;
    std::mutex connections_mutex_;
    
    // 分布式事务状态
    std::mutex transaction_mutex_;
    bool in_distributed_transaction_;
    
    static std::once_flag initialized_;
    static std::unique_ptr<MultiConnectionDatabaseManager> instance_;
};

// 自定义删除器用于sqlite3指针
struct SQLiteDeleter {
    void operator()(sqlite3* db) {
        if (db) {
            sqlite3_close_v2(db);
        }
    }
};

// 分布式事务RAII管理器
class DistributedTransaction {
public:
    DistributedTransaction();
    ~DistributedTransaction();
    
    bool commit();
    void rollback();
    
private:
    MultiConnectionDatabaseManager& db_manager_;
    bool committed_;
    bool rolled_back_;
};
