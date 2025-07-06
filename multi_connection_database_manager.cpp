#include "multi_connection_database_manager.h"
#include <iostream>

std::once_flag MultiConnectionDatabaseManager::initialized_;
std::unique_ptr<MultiConnectionDatabaseManager> MultiConnectionDatabaseManager::instance_;

MultiConnectionDatabaseManager& MultiConnectionDatabaseManager::getInstance() {
    std::call_once(initialized_, []() {
        instance_ = std::unique_ptr<MultiConnectionDatabaseManager>(new MultiConnectionDatabaseManager());
    });
    return *instance_;
}

MultiConnectionDatabaseManager::MultiConnectionDatabaseManager() 
    : in_distributed_transaction_(false) {
    
    // 为每个表设置独立的数据库文件
    db_paths_[TableType::USERS] = "users_db.db";
    db_paths_[TableType::ORDERS] = "orders_db.db";
    db_paths_[TableType::PRODUCTS] = "products_db.db";
    
    // 打开所有连接
    for (const auto& pair : db_paths_) {
        openConnection(pair.first, pair.second);
    }
    
    // 初始化所有表
    initializeAllTables();
}

MultiConnectionDatabaseManager::~MultiConnectionDatabaseManager() {
    // shared_ptr会自动处理数据库连接的关闭
}

void MultiConnectionDatabaseManager::openConnection(TableType table, const std::string& db_path) {
    sqlite3* raw_db = nullptr;
    
    // 使用NOMUTEX模式获得最佳性能
    int result = sqlite3_open_v2(
        db_path.c_str(),
        &raw_db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
        nullptr
    );
    
    if (result != SQLITE_OK) {
        std::string error_msg = "无法打开数据库 " + db_path + ": ";
        if (raw_db) {
            error_msg += sqlite3_errmsg(raw_db);
            sqlite3_close(raw_db);
        }
        throw std::runtime_error(error_msg);
    }
    
    // 配置数据库
    configureConnection(raw_db);
    
    // 使用shared_ptr管理数据库连接
    connections_[table] = std::shared_ptr<sqlite3>(raw_db, SQLiteDeleter());
    
    std::cout << "已打开数据库连接: " << db_path << std::endl;
}

void MultiConnectionDatabaseManager::configureConnection(sqlite3* db) {
    char* error_msg = nullptr;
    
    // 启用WAL模式以提高并发性能
    int result = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        std::string error = "设置WAL模式失败: " + std::string(error_msg);
        sqlite3_free(error_msg);
        throw std::runtime_error(error);
    }
    
    // 设置同步模式为NORMAL以平衡性能和安全性
    result = sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        std::string error = "设置同步模式失败: " + std::string(error_msg);
        sqlite3_free(error_msg);
        throw std::runtime_error(error);
    }
    
    // 设置缓存大小
    result = sqlite3_exec(db, "PRAGMA cache_size=5000;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        std::string error = "设置缓存大小失败: " + std::string(error_msg);
        sqlite3_free(error_msg);
        throw std::runtime_error(error);
    }
    
    // 设置临时存储为内存
    result = sqlite3_exec(db, "PRAGMA temp_store=MEMORY;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        std::string error = "设置临时存储失败: " + std::string(error_msg);
        sqlite3_free(error_msg);
        throw std::runtime_error(error);
    }
    
    // 设置忙等待超时
    sqlite3_busy_timeout(db, 30000); // 30秒
}

std::shared_ptr<sqlite3> MultiConnectionDatabaseManager::getConnection(TableType table) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(table);
    if (it != connections_.end()) {
        return it->second;
    }
    throw std::runtime_error("未找到指定表的数据库连接");
}

void MultiConnectionDatabaseManager::initializeAllTables() {
    initializeTable(TableType::USERS);
    initializeTable(TableType::ORDERS);
    initializeTable(TableType::PRODUCTS);
    std::cout << "所有数据库表初始化完成" << std::endl;
}

void MultiConnectionDatabaseManager::initializeTable(TableType table) {
    auto db = getConnection(table).get();
    char* error_msg = nullptr;
    const char* create_sql = nullptr;
    
    switch (table) {
        case TableType::USERS:
            create_sql = R"(
                CREATE TABLE IF NOT EXISTS users (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    username TEXT UNIQUE NOT NULL,
                    email TEXT UNIQUE NOT NULL,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
                );
                CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
                CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
            )";
            break;
            
        case TableType::ORDERS:
            create_sql = R"(
                CREATE TABLE IF NOT EXISTS orders (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    user_id INTEGER NOT NULL,
                    total_amount DECIMAL(10,2) NOT NULL,
                    status TEXT DEFAULT 'pending',
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
                );
                CREATE INDEX IF NOT EXISTS idx_orders_user_id ON orders(user_id);
                CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);
            )";
            break;
            
        case TableType::PRODUCTS:
            create_sql = R"(
                CREATE TABLE IF NOT EXISTS products (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT NOT NULL,
                    description TEXT,
                    price DECIMAL(10,2) NOT NULL,
                    stock_quantity INTEGER DEFAULT 0,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
                );
                CREATE INDEX IF NOT EXISTS idx_products_name ON products(name);
                CREATE INDEX IF NOT EXISTS idx_products_price ON products(price);
            )";
            break;
    }
    
    int result = sqlite3_exec(db, create_sql, nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        std::string error = "创建表失败: " + std::string(error_msg);
        sqlite3_free(error_msg);
        throw std::runtime_error(error);
    }
}

bool MultiConnectionDatabaseManager::beginDistributedTransaction() {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (in_distributed_transaction_) {
        return false; // 已经在事务中
    }
    
    // 在所有连接上开始事务
    for (const auto& pair : connections_) {
        char* error_msg = nullptr;
        int result = sqlite3_exec(pair.second.get(), "BEGIN IMMEDIATE;", nullptr, nullptr, &error_msg);
        if (result != SQLITE_OK) {
            // 回滚已经开始的事务
            rollbackDistributedTransaction();
            sqlite3_free(error_msg);
            return false;
        }
    }
    
    in_distributed_transaction_ = true;
    return true;
}

bool MultiConnectionDatabaseManager::commitDistributedTransaction() {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (!in_distributed_transaction_) {
        return false;
    }
    
    // 在所有连接上提交事务
    for (const auto& pair : connections_) {
        char* error_msg = nullptr;
        int result = sqlite3_exec(pair.second.get(), "COMMIT;", nullptr, nullptr, &error_msg);
        if (result != SQLITE_OK) {
            // 如果提交失败，回滚所有事务
            rollbackDistributedTransaction();
            sqlite3_free(error_msg);
            return false;
        }
    }
    
    in_distributed_transaction_ = false;
    return true;
}

bool MultiConnectionDatabaseManager::rollbackDistributedTransaction() {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (!in_distributed_transaction_) {
        return false;
    }
    
    // 在所有连接上回滚事务
    for (const auto& pair : connections_) {
        sqlite3_exec(pair.second.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
    }
    
    in_distributed_transaction_ = false;
    return true;
}

// 分布式事务RAII管理器实现
DistributedTransaction::DistributedTransaction() 
    : db_manager_(MultiConnectionDatabaseManager::getInstance()),
      committed_(false), rolled_back_(false) {
    
    if (!db_manager_.beginDistributedTransaction()) {
        throw std::runtime_error("无法开始分布式事务");
    }
}

DistributedTransaction::~DistributedTransaction() {
    if (!committed_ && !rolled_back_) {
        rollback();
    }
}

bool DistributedTransaction::commit() {
    if (committed_ || rolled_back_) {
        return false;
    }
    
    committed_ = db_manager_.commitDistributedTransaction();
    return committed_;
}

void DistributedTransaction::rollback() {
    if (!committed_ && !rolled_back_) {
        rolled_back_ = true;
        db_manager_.rollbackDistributedTransaction();
    }
}
