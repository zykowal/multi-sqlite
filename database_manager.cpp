#include "database_manager.h"
#include <iostream>

std::once_flag DatabaseManager::initialized_;
std::unique_ptr<DatabaseManager> DatabaseManager::instance_;

DatabaseManager& DatabaseManager::getInstance() {
    std::call_once(initialized_, []() {
        instance_ = std::unique_ptr<DatabaseManager>(new DatabaseManager());
    });
    return *instance_;
}

DatabaseManager::DatabaseManager() {
    openDatabase();
    configureDatabase();
    initializeTables();
}

DatabaseManager::~DatabaseManager() {
    // shared_ptr会自动处理数据库连接的关闭
}

void DatabaseManager::openDatabase() {
    sqlite3* raw_db = nullptr;
    
    // 使用SQLITE_OPEN_FULLMUTEX确保线程安全
    int result = sqlite3_open_v2(
        db_path_.c_str(),
        &raw_db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );
    
    if (result != SQLITE_OK) {
        std::string error_msg = "无法打开数据库: ";
        if (raw_db) {
            error_msg += sqlite3_errmsg(raw_db);
            sqlite3_close(raw_db);
        }
        throw std::runtime_error(error_msg);
    }
    
    // 使用shared_ptr管理数据库连接
    db_connection_ = std::shared_ptr<sqlite3>(raw_db, SQLiteDeleter());
}

void DatabaseManager::configureDatabase() {
    auto db = db_connection_.get();
    
    // 启用WAL模式以提高并发性能
    char* error_msg = nullptr;
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
    result = sqlite3_exec(db, "PRAGMA cache_size=10000;", nullptr, nullptr, &error_msg);
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

std::shared_ptr<sqlite3> DatabaseManager::getConnection() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    return db_connection_;
}

void DatabaseManager::initializeTables() {
    auto db = db_connection_.get();
    char* error_msg = nullptr;
    
    // 创建用户表
    const char* create_users_table = R"(
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
    
    // 创建订单表
    const char* create_orders_table = R"(
        CREATE TABLE IF NOT EXISTS orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            total_amount DECIMAL(10,2) NOT NULL,
            status TEXT DEFAULT 'pending',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id)
        );
        CREATE INDEX IF NOT EXISTS idx_orders_user_id ON orders(user_id);
        CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);
    )";
    
    // 创建产品表
    const char* create_products_table = R"(
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
    
    // 执行表创建语句
    int result = sqlite3_exec(db, create_users_table, nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        std::string error = "创建用户表失败: " + std::string(error_msg);
        sqlite3_free(error_msg);
        throw std::runtime_error(error);
    }
    
    result = sqlite3_exec(db, create_orders_table, nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        std::string error = "创建订单表失败: " + std::string(error_msg);
        sqlite3_free(error_msg);
        throw std::runtime_error(error);
    }
    
    result = sqlite3_exec(db, create_products_table, nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        std::string error = "创建产品表失败: " + std::string(error_msg);
        sqlite3_free(error_msg);
        throw std::runtime_error(error);
    }
    
    std::cout << "数据库表初始化完成" << std::endl;
}
