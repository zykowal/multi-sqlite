#include "sqlite_wrapper.h"
#include <iostream>

SQLiteWrapper::SQLiteWrapper(const std::string& db_path) : db_(nullptr) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        last_error_ = "无法打开数据库: " + std::string(sqlite3_errmsg(db_));
        std::cerr << last_error_ << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

SQLiteWrapper::~SQLiteWrapper() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SQLiteWrapper::createTable(const std::string& table_name, const std::string& columns_def) {
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table_name + " (" + columns_def + ");";
    return executeSQL(sql);
}

bool SQLiteWrapper::insert(const std::string& table_name, const std::string& columns, const std::string& values) {
    std::string sql = "INSERT INTO " + table_name + " (" + columns + ") VALUES (" + values + ");";
    return executeSQL(sql);
}

bool SQLiteWrapper::update(const std::string& table_name, const std::string& set_clause, const std::string& where_clause) {
    std::string sql = "UPDATE " + table_name + " SET " + set_clause;
    if (!where_clause.empty()) {
        sql += " WHERE " + where_clause;
    }
    sql += ";";
    return executeSQL(sql);
}

bool SQLiteWrapper::remove(const std::string& table_name, const std::string& where_clause) {
    std::string sql = "DELETE FROM " + table_name;
    if (!where_clause.empty()) {
        sql += " WHERE " + where_clause;
    }
    sql += ";";
    return executeSQL(sql);
}

// 查询回调函数
static int queryCallback(void* data, int argc, char** argv, char** col_names) {
    auto callback = static_cast<SQLiteWrapper::QueryCallback*>(data);
    if (callback) {
        (*callback)(argc, argv, col_names);
    }
    return 0;
}

bool SQLiteWrapper::query(const std::string& sql, QueryCallback callback) {
    if (!db_) {
        last_error_ = "数据库未打开";
        return false;
    }
    
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), queryCallback, &callback, &err_msg);
    
    if (rc != SQLITE_OK) {
        last_error_ = "SQL执行错误: " + std::string(err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool SQLiteWrapper::executeSQL(const std::string& sql) {
    if (!db_) {
        last_error_ = "数据库未打开";
        return false;
    }
    
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        last_error_ = "SQL执行错误: " + std::string(err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

std::string SQLiteWrapper::getLastError() const {
    return last_error_;
}

// 实现C风格导出函数
extern "C" {
    // 创建SQLiteWrapper实例
    SQLITE_WRAPPER_API SQLiteWrapper* CreateSQLiteWrapper(const char* db_path) {
        return new SQLiteWrapper(db_path);
    }
    
    // 销毁SQLiteWrapper实例
    SQLITE_WRAPPER_API void DestroySQLiteWrapper(SQLiteWrapper* wrapper) {
        delete wrapper;
    }
    
    // 创建表
    SQLITE_WRAPPER_API bool CreateTable(SQLiteWrapper* wrapper, const char* table_name, const char* columns_def) {
        if (!wrapper) return false;
        return wrapper->createTable(table_name, columns_def);
    }
    
    // 插入数据
    SQLITE_WRAPPER_API bool Insert(SQLiteWrapper* wrapper, const char* table_name, const char* columns, const char* values) {
        if (!wrapper) return false;
        return wrapper->insert(table_name, columns, values);
    }
    
    // 更新数据
    SQLITE_WRAPPER_API bool Update(SQLiteWrapper* wrapper, const char* table_name, const char* set_clause, const char* where_clause) {
        if (!wrapper) return false;
        return wrapper->update(table_name, set_clause, where_clause ? where_clause : "");
    }
    
    // 删除数据
    SQLITE_WRAPPER_API bool Remove(SQLiteWrapper* wrapper, const char* table_name, const char* where_clause) {
        if (!wrapper) return false;
        return wrapper->remove(table_name, where_clause ? where_clause : "");
    }
    
    // 获取最后错误
    SQLITE_WRAPPER_API const char* GetLastError(SQLiteWrapper* wrapper) {
        if (!wrapper) return "无效的SQLiteWrapper实例";
        static std::string error;
        error = wrapper->getLastError();
        return error.c_str();
    }
    
    // 查询数据的C回调包装
    struct QueryCallbackWrapper {
        QueryCallbackFunc func;
        void* user_data;
    };
    
    static void cppQueryCallback(int argc, char** argv, char** col_names) {
        auto* wrapper = static_cast<QueryCallbackWrapper*>(nullptr);
        if (wrapper && wrapper->func) {
            wrapper->func(argc, argv, col_names, wrapper->user_data);
        }
    }
    
    // 查询数据
    SQLITE_WRAPPER_API bool Query(SQLiteWrapper* wrapper, const char* sql, QueryCallbackFunc callback, void* user_data) {
        if (!wrapper) return false;
        
        if (!callback) {
            return wrapper->query(sql, [](int, char**, char**) {});
        }
        
        return wrapper->query(sql, [callback, user_data](int argc, char** argv, char** col_names) {
            callback(argc, argv, col_names, user_data);
        });
    }
}
