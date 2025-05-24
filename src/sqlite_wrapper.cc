#include "sqlite_wrapper.h"
#include <iostream>

// PIMPL实现类
class SQLiteWrapperImpl {
public:
  explicit SQLiteWrapperImpl(std::string_view db_path) : db_(nullptr) {
    int rc = sqlite3_open(std::string(db_path).c_str(), &db_);
    if (rc != SQLITE_OK) {
      last_error_ = "无法打开数据库: " + std::string(sqlite3_errmsg(db_));
      std::cerr << last_error_ << std::endl;
      sqlite3_close(db_);
      db_ = nullptr;
    }
  }

  ~SQLiteWrapperImpl() {
    if (db_) {
      sqlite3_close(db_);
      db_ = nullptr;
    }
  }

  bool executeSQL(std::string_view sql) {
    if (!db_) {
      last_error_ = "数据库未打开";
      return false;
    }

    char *err_msg = nullptr;
    int rc =
        sqlite3_exec(db_, std::string(sql).c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
      last_error_ = "SQL执行错误: " + std::string(err_msg);
      sqlite3_free(err_msg);
      return false;
    }

    return true;
  }

  // 查询回调结构体
  struct QueryCallbackData {
    std::vector<std::string> column_names;
    std::vector<std::vector<std::string>> rows;
    ISQLiteWrapper::QueryCallback callback;
  };

  // 查询回调函数
  static int queryCallback(void *data, int argc, char **argv,
                           char **col_names) {
    auto *callback_data = static_cast<QueryCallbackData *>(data);

    // 如果是第一行，保存列名
    if (callback_data->rows.empty() && callback_data->column_names.empty()) {
      for (int i = 0; i < argc; i++) {
        callback_data->column_names.emplace_back(col_names[i] ? col_names[i]
                                                              : "");
      }
    }

    // 保存行数据
    std::vector<std::string> row;
    for (int i = 0; i < argc; i++) {
      row.emplace_back(argv[i] ? argv[i] : "NULL");
    }
    callback_data->rows.push_back(std::move(row));

    // 如果有回调函数，调用它
    if (callback_data->callback) {
      callback_data->callback(callback_data->column_names, row);
    }

    return 0;
  }

  std::optional<QueryResult> query(std::string_view sql) {
    if (!db_) {
      last_error_ = "数据库未打开";
      return std::nullopt;
    }

    QueryCallbackData callback_data;
    char *err_msg = nullptr;
    int rc = sqlite3_exec(db_, std::string(sql).c_str(), queryCallback,
                          &callback_data, &err_msg);

    if (rc != SQLITE_OK) {
      last_error_ = "SQL执行错误: " + std::string(err_msg);
      sqlite3_free(err_msg);
      return std::nullopt;
    }

    QueryResult result;
    result.column_names = std::move(callback_data.column_names);
    result.rows = std::move(callback_data.rows);
    return result;
  }

  bool queryWithCallback(std::string_view sql,
                         ISQLiteWrapper::QueryCallback callback) {
    if (!db_) {
      last_error_ = "数据库未打开";
      return false;
    }

    QueryCallbackData callback_data;
    callback_data.callback = std::move(callback);

    char *err_msg = nullptr;
    int rc = sqlite3_exec(db_, std::string(sql).c_str(), queryCallback,
                          &callback_data, &err_msg);

    if (rc != SQLITE_OK) {
      last_error_ = "SQL执行错误: " + std::string(err_msg);
      sqlite3_free(err_msg);
      return false;
    }

    return true;
  }

  std::string_view getLastError() const { return last_error_; }

  sqlite3 *db_;
  std::string last_error_;
};

// SQLiteWrapper实现

SQLiteWrapper::SQLiteWrapper(std::string_view db_path)
    : pimpl_(std::make_unique<SQLiteWrapperImpl>(db_path)) {}

SQLiteWrapper::SQLiteWrapper(SQLiteWrapper &&other) noexcept = default;
SQLiteWrapper &
SQLiteWrapper::operator=(SQLiteWrapper &&other) noexcept = default;
SQLiteWrapper::~SQLiteWrapper() = default;

bool SQLiteWrapper::createTable(std::string_view table_name,
                                std::string_view columns_def) {
  std::string sql = "CREATE TABLE IF NOT EXISTS " + std::string(table_name) +
                    " (" + std::string(columns_def) + ");";
  return pimpl_->executeSQL(sql);
}

bool SQLiteWrapper::insert(std::string_view table_name,
                           std::string_view columns, std::string_view values) {
  std::string sql = "INSERT INTO " + std::string(table_name) + " (" +
                    std::string(columns) + ") VALUES (" + std::string(values) +
                    ");";
  return pimpl_->executeSQL(sql);
}

bool SQLiteWrapper::update(std::string_view table_name,
                           std::string_view set_clause,
                           std::string_view where_clause) {
  std::string sql =
      "UPDATE " + std::string(table_name) + " SET " + std::string(set_clause);
  if (!where_clause.empty()) {
    sql += " WHERE " + std::string(where_clause);
  }
  sql += ";";
  return pimpl_->executeSQL(sql);
}

bool SQLiteWrapper::remove(std::string_view table_name,
                           std::string_view where_clause) {
  std::string sql = "DELETE FROM " + std::string(table_name);
  if (!where_clause.empty()) {
    sql += " WHERE " + std::string(where_clause);
  }
  sql += ";";
  return pimpl_->executeSQL(sql);
}

std::optional<QueryResult> SQLiteWrapper::query(std::string_view sql) {
  return pimpl_->query(sql);
}

bool SQLiteWrapper::queryWithCallback(std::string_view sql,
                                      QueryCallback callback) {
  return pimpl_->queryWithCallback(sql, std::move(callback));
}

std::string_view SQLiteWrapper::getLastError() const {
  return pimpl_->getLastError();
}

bool SQLiteWrapper::beginTransaction() {
  return pimpl_->executeSQL("BEGIN TRANSACTION;");
}

bool SQLiteWrapper::commitTransaction() {
  return pimpl_->executeSQL("COMMIT;");
}

bool SQLiteWrapper::rollbackTransaction() {
  return pimpl_->executeSQL("ROLLBACK;");
}

// 工厂函数实现
extern "C" {
SQLITE_WRAPPER_API ISQLiteWrapper *CreateSQLiteWrapper(const char *db_path) {
  return new SQLiteWrapper(db_path);
}

SQLITE_WRAPPER_API void DestroySQLiteWrapper(ISQLiteWrapper *wrapper) {
  delete wrapper;
}
}
