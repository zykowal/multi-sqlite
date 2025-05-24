#ifndef SQLITE_WRAPPER_H
#define SQLITE_WRAPPER_H

#include <memory>
#include <optional>
#include <sqlite3.h>
#include <string_view>

// 导入接口定义
#include "../app/sqlite_wrapper_interface.h"

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

// 前向声明实现类
class SQLiteWrapperImpl;

// SQLiteWrapper类实现接口
class SQLITE_WRAPPER_API SQLiteWrapper : public ISQLiteWrapper {
public:
  // 构造函数，打开数据库连接
  explicit SQLiteWrapper(std::string_view db_path);

  // 移动构造函数
  SQLiteWrapper(SQLiteWrapper &&other) noexcept;

  // 移动赋值运算符
  SQLiteWrapper &operator=(SQLiteWrapper &&other) noexcept;

  // 禁用拷贝
  SQLiteWrapper(const SQLiteWrapper &) = delete;
  SQLiteWrapper &operator=(const SQLiteWrapper &) = delete;

  // 析构函数
  ~SQLiteWrapper() override;

  // 接口实现
  bool createTable(std::string_view table_name,
                   std::string_view columns_def) override;
  bool insert(std::string_view table_name, std::string_view columns,
              std::string_view values) override;
  bool update(std::string_view table_name, std::string_view set_clause,
              std::string_view where_clause = "") override;
  bool remove(std::string_view table_name,
              std::string_view where_clause = "") override;
  std::optional<QueryResult> query(std::string_view sql) override;
  bool queryWithCallback(std::string_view sql, QueryCallback callback) override;
  std::string_view getLastError() const override;
  bool beginTransaction() override;
  bool commitTransaction() override;
  bool rollbackTransaction() override;

private:
  // PIMPL模式实现
  std::unique_ptr<SQLiteWrapperImpl> pimpl_;
};

#endif // SQLITE_WRAPPER_H
