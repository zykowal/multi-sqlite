#ifndef SQLITE_WRAPPER_INTERFACE_H
#define SQLITE_WRAPPER_INTERFACE_H

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// 查询结果类型
struct QueryResult {
  std::vector<std::string> column_names;
  std::vector<std::vector<std::string>> rows;
};

// 定义接口类
class ISQLiteWrapper {
public:
  virtual ~ISQLiteWrapper() = default;

  // 创建表
  virtual bool createTable(std::string_view table_name,
                           std::string_view columns_def) = 0;

  // 插入数据
  virtual bool insert(std::string_view table_name, std::string_view columns,
                      std::string_view values) = 0;

  // 更新数据
  virtual bool update(std::string_view table_name, std::string_view set_clause,
                      std::string_view where_clause = "") = 0;

  // 删除数据
  virtual bool remove(std::string_view table_name,
                      std::string_view where_clause = "") = 0;

  // 查询数据 - 返回结果集
  virtual std::optional<QueryResult> query(std::string_view sql) = 0;

  // 查询数据 - 使用回调函数
  using QueryCallback =
      std::function<void(const std::vector<std::string> &column_names,
                         const std::vector<std::string> &row_values)>;
  virtual bool queryWithCallback(std::string_view sql,
                                 QueryCallback callback) = 0;

  // 获取最后一个错误信息
  virtual std::string_view getLastError() const = 0;

  // 事务支持
  virtual bool beginTransaction() = 0;
  virtual bool commitTransaction() = 0;
  virtual bool rollbackTransaction() = 0;
};

// 工厂函数类型定义
extern "C" {
ISQLiteWrapper *CreateSQLiteWrapper(const char *db_path);
void DestroySQLiteWrapper(ISQLiteWrapper *wrapper);
}

#endif // SQLITE_WRAPPER_INTERFACE_H
