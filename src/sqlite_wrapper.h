#ifndef SQLITE_WRAPPER_H
#define SQLITE_WRAPPER_H

#include <functional>
#include <memory>
#include <sqlite3.h>
#include <string>

// 定义导出宏，用于动态库导出符号
#if defined(_WIN32) || defined(_WIN64)
    #ifdef SQLITE_WRAPPER_EXPORTS
        #define SQLITE_WRAPPER_API __declspec(dllexport)
    #else
        #define SQLITE_WRAPPER_API __declspec(dllimport)
    #endif
#else
    #define SQLITE_WRAPPER_API __attribute__((visibility("default")))
#endif

class SQLITE_WRAPPER_API SQLiteWrapper {
public:
  // 构造函数，打开数据库连接
  SQLiteWrapper(const std::string &db_path);

  // 析构函数，关闭数据库连接
  ~SQLiteWrapper();

  // 创建表
  bool createTable(const std::string &table_name,
                   const std::string &columns_def);

  // 插入数据
  bool insert(const std::string &table_name, const std::string &columns,
              const std::string &values);

  // 更新数据
  bool update(const std::string &table_name, const std::string &set_clause,
              const std::string &where_clause);

  // 删除数据
  bool remove(const std::string &table_name, const std::string &where_clause);

  // 查询数据
  using QueryCallback =
      std::function<void(int argc, char **argv, char **col_names)>;
  bool query(const std::string &sql, QueryCallback callback);

  // 获取最后一个错误信息
  std::string getLastError() const;

private:
  sqlite3 *db_;
  std::string last_error_;

  // 执行SQL语句
  bool executeSQL(const std::string &sql);
};

// 定义导出的C风格函数，用于动态加载
extern "C" {
    SQLITE_WRAPPER_API SQLiteWrapper* CreateSQLiteWrapper(const char* db_path);
    SQLITE_WRAPPER_API void DestroySQLiteWrapper(SQLiteWrapper* wrapper);
    SQLITE_WRAPPER_API bool CreateTable(SQLiteWrapper* wrapper, const char* table_name, const char* columns_def);
    SQLITE_WRAPPER_API bool Insert(SQLiteWrapper* wrapper, const char* table_name, const char* columns, const char* values);
    SQLITE_WRAPPER_API bool Update(SQLiteWrapper* wrapper, const char* table_name, const char* set_clause, const char* where_clause);
    SQLITE_WRAPPER_API bool Remove(SQLiteWrapper* wrapper, const char* table_name, const char* where_clause);
    SQLITE_WRAPPER_API const char* GetLastError(SQLiteWrapper* wrapper);
    
    // 查询回调函数类型定义
    typedef void (*QueryCallbackFunc)(int argc, char** argv, char** col_names, void* user_data);
    SQLITE_WRAPPER_API bool Query(SQLiteWrapper* wrapper, const char* sql, QueryCallbackFunc callback, void* user_data);
}

#endif // SQLITE_WRAPPER_H
