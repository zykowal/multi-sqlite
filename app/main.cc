#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "sqlite_wrapper_interface.h"

namespace fs = std::filesystem;

// 定义动态库加载器类
class DynamicLibrary {
public:
  explicit DynamicLibrary(const std::string &path) : handle_(nullptr) {
    handle_ = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle_) {
      throw std::runtime_error(std::string("无法加载动态库: ") + dlerror());
    }
    // 清除可能存在的错误
    dlerror();
  }

  ~DynamicLibrary() {
    if (handle_) {
      dlclose(handle_);
    }
  }

  // 禁用拷贝
  DynamicLibrary(const DynamicLibrary &) = delete;
  DynamicLibrary &operator=(const DynamicLibrary &) = delete;

  // 获取函数指针
  template <typename T> T getSymbol(const std::string &name) const {
    void *symbol = dlsym(handle_, name.c_str());
    const char *error = dlerror();
    if (error) {
      throw std::runtime_error(std::string("加载函数失败: ") + error);
    }
    return reinterpret_cast<T>(symbol);
  }

private:
  void *handle_;
};

// 定义函数指针类型
using CreateSQLiteWrapperFunc = ISQLiteWrapper *(*)(const char *);
using DestroySQLiteWrapperFunc = void (*)(ISQLiteWrapper *);

// 打印查询结果的辅助函数
void printQueryResults(const std::vector<std::string> &column_names,
                       const std::vector<std::string> &row_values) {
  for (size_t i = 0; i < column_names.size() && i < row_values.size(); ++i) {
    std::cout << column_names[i] << ": " << row_values[i] << "\t";
  }
  std::cout << std::endl;
}

int main(int argc, char **argv) {
  std::cout << "SQLite 现代C++动态加载库示例" << std::endl;

  try {
    // 获取当前可执行文件路径
    fs::path lib_path = "../src/libsqlite_wrapper.dylib";

    std::cout << "加载动态库: " << lib_path << std::endl;

    // 加载动态库
    DynamicLibrary lib(lib_path.string());

    // 获取创建和销毁SQLiteWrapper的函数
    auto createWrapper =
        lib.getSymbol<CreateSQLiteWrapperFunc>("CreateSQLiteWrapper");
    auto destroyWrapper =
        lib.getSymbol<DestroySQLiteWrapperFunc>("DestroySQLiteWrapper");

    // 创建SQLiteWrapper实例并使用智能指针管理
    std::unique_ptr<ISQLiteWrapper, DestroySQLiteWrapperFunc> db(
        createWrapper("test_dynamic_modern.db"), destroyWrapper);

    // 创建表
    std::string table_name = "users";
    std::string columns_def = "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "name TEXT NOT NULL, "
                              "age INTEGER, "
                              "email TEXT";

    if (!db->createTable(table_name, columns_def)) {
      std::cerr << "创建表失败: " << db->getLastError() << std::endl;
      return 1;
    }
    std::cout << "表创建成功" << std::endl;

    // 插入数据
    if (!db->insert(table_name, "name, age, email",
                    "'张三', 25, 'zhangsan@example.com'")) {
      std::cerr << "插入数据失败: " << db->getLastError() << std::endl;
      return 1;
    }
    std::cout << "插入数据成功" << std::endl;

    if (!db->insert(table_name, "name, age, email",
                    "'李四', 30, 'lisi@example.com'")) {
      std::cerr << "插入数据失败: " << db->getLastError() << std::endl;
      return 1;
    }
    std::cout << "插入数据成功" << std::endl;

    // 查询数据 - 使用回调函数
    std::cout << "查询所有用户 (使用回调):" << std::endl;
    std::string sql = "SELECT * FROM " + table_name + ";";
    if (!db->queryWithCallback(sql, printQueryResults)) {
      std::cerr << "查询数据失败: " << db->getLastError() << std::endl;
      return 1;
    }

    // 查询数据 - 使用返回结果集
    std::cout << "\n查询所有用户 (使用结果集):" << std::endl;
    auto result = db->query(sql);
    if (!result) {
      std::cerr << "查询数据失败: " << db->getLastError() << std::endl;
      return 1;
    }

    // 打印结果集
    for (const auto &row : result->rows) {
      for (size_t i = 0; i < result->column_names.size() && i < row.size();
           ++i) {
        std::cout << result->column_names[i] << ": " << row[i] << "\t";
      }
      std::cout << std::endl;
    }

    // 更新数据
    if (!db->update(table_name, "age = 26", "name = '张三'")) {
      std::cerr << "更新数据失败: " << db->getLastError() << std::endl;
      return 1;
    }
    std::cout << "\n更新数据成功" << std::endl;

    // 再次查询数据
    std::cout << "更新后查询所有用户:" << std::endl;
    result = db->query(sql);
    if (!result) {
      std::cerr << "查询数据失败: " << db->getLastError() << std::endl;
      return 1;
    }

    // 打印结果集
    for (const auto &row : result->rows) {
      for (size_t i = 0; i < result->column_names.size() && i < row.size();
           ++i) {
        std::cout << result->column_names[i] << ": " << row[i] << "\t";
      }
      std::cout << std::endl;
    }

    // 删除数据
    if (!db->remove(table_name, "name = '李四'")) {
      std::cerr << "删除数据失败: " << db->getLastError() << std::endl;
      return 1;
    }
    std::cout << "\n删除数据成功" << std::endl;

    // 最后查询数据
    std::cout << "删除后查询所有用户:" << std::endl;
    result = db->query(sql);
    if (!result) {
      std::cerr << "查询数据失败: " << db->getLastError() << std::endl;
      return 1;
    }

    // 打印结果集
    for (const auto &row : result->rows) {
      for (size_t i = 0; i < result->column_names.size() && i < row.size();
           ++i) {
        std::cout << result->column_names[i] << ": " << row[i] << "\t";
      }
      std::cout << std::endl;
    }

    // 事务示例
    std::cout << "\n事务示例:" << std::endl;
    if (db->beginTransaction()) {
      bool success = true;

      // 在事务中执行多个操作
      success &= db->insert(table_name, "name, age, email",
                            "'王五', 35, 'wangwu@example.com'");
      success &= db->insert(table_name, "name, age, email",
                            "'赵六', 40, 'zhaoliu@example.com'");

      if (success) {
        if (db->commitTransaction()) {
          std::cout << "事务提交成功" << std::endl;
        } else {
          std::cerr << "事务提交失败: " << db->getLastError() << std::endl;
        }
      } else {
        if (db->rollbackTransaction()) {
          std::cout << "事务回滚成功" << std::endl;
        } else {
          std::cerr << "事务回滚失败: " << db->getLastError() << std::endl;
        }
      }
    }

    // 最终查询数据
    std::cout << "\n最终查询所有用户:" << std::endl;
    result = db->query(sql);
    if (result) {
      for (const auto &row : result->rows) {
        for (size_t i = 0; i < result->column_names.size() && i < row.size();
             ++i) {
          std::cout << result->column_names[i] << ": " << row[i] << "\t";
        }
        std::cout << std::endl;
      }
    }

  } catch (const std::exception &e) {
    std::cerr << "错误: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
