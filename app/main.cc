#include <dlfcn.h>
#include <iostream>
#include <string>

// 定义函数指针类型
typedef void *(*CreateSQLiteWrapperFunc)(const char *);
typedef void (*DestroySQLiteWrapperFunc)(void *);
typedef bool (*CreateTableFunc)(void *, const char *, const char *);
typedef bool (*InsertFunc)(void *, const char *, const char *, const char *);
typedef bool (*UpdateFunc)(void *, const char *, const char *, const char *);
typedef bool (*RemoveFunc)(void *, const char *, const char *);
typedef const char *(*GetLastErrorFunc)(void *);
typedef void (*QueryCallbackFunc)(int, char **, char **, void *);
typedef bool (*QueryFunc)(void *, const char *, QueryCallbackFunc, void *);

// 打印查询结果的回调函数
void printQueryResults(int argc, char **argv, char **col_names,
                       void *user_data) {
  for (int i = 0; i < argc; i++) {
    std::cout << col_names[i] << ": " << (argv[i] ? argv[i] : "NULL") << "\t";
  }
  std::cout << std::endl;
}

int main(int argc, char **argv) {
  std::cout << "SQLite 动态加载库示例" << std::endl;

  // 加载动态库
  void *handle = dlopen("../src/libsqlite_wrapper.dylib", RTLD_LAZY);
  if (!handle) {
    std::cerr << "无法加载动态库: " << dlerror() << std::endl;
    return 1;
  }

  // 清除可能存在的错误
  dlerror();

  // 获取函数指针
  CreateSQLiteWrapperFunc createWrapper =
      (CreateSQLiteWrapperFunc)dlsym(handle, "CreateSQLiteWrapper");
  DestroySQLiteWrapperFunc destroyWrapper =
      (DestroySQLiteWrapperFunc)dlsym(handle, "DestroySQLiteWrapper");
  CreateTableFunc createTable = (CreateTableFunc)dlsym(handle, "CreateTable");
  InsertFunc insert = (InsertFunc)dlsym(handle, "Insert");
  UpdateFunc update = (UpdateFunc)dlsym(handle, "Update");
  RemoveFunc remove = (RemoveFunc)dlsym(handle, "Remove");
  GetLastErrorFunc getLastError =
      (GetLastErrorFunc)dlsym(handle, "GetLastError");
  QueryFunc query = (QueryFunc)dlsym(handle, "Query");

  // 检查是否所有函数都成功加载
  const char *error = dlerror();
  if (error) {
    std::cerr << "加载函数失败: " << error << std::endl;
    dlclose(handle);
    return 1;
  }

  // 创建SQLiteWrapper实例
  void *db = createWrapper("test_dynamic.db");
  if (!db) {
    std::cerr << "创建SQLiteWrapper实例失败" << std::endl;
    dlclose(handle);
    return 1;
  }

  // 创建表
  std::string table_name = "users";
  std::string columns_def = "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "name TEXT NOT NULL, "
                            "age INTEGER, "
                            "email TEXT";

  if (!createTable(db, table_name.c_str(), columns_def.c_str())) {
    std::cerr << "创建表失败: " << getLastError(db) << std::endl;
    destroyWrapper(db);
    dlclose(handle);
    return 1;
  }
  std::cout << "表创建成功" << std::endl;

  // 插入数据
  if (!insert(db, table_name.c_str(), "name, age, email",
              "'张三', 25, 'zhangsan@example.com'")) {
    std::cerr << "插入数据失败: " << getLastError(db) << std::endl;
    destroyWrapper(db);
    dlclose(handle);
    return 1;
  }
  std::cout << "插入数据成功" << std::endl;

  if (!insert(db, table_name.c_str(), "name, age, email",
              "'李四', 30, 'lisi@example.com'")) {
    std::cerr << "插入数据失败: " << getLastError(db) << std::endl;
    destroyWrapper(db);
    dlclose(handle);
    return 1;
  }
  std::cout << "插入数据成功" << std::endl;

  // 查询数据
  std::cout << "查询所有用户:" << std::endl;
  std::string sql = "SELECT * FROM " + table_name + ";";
  if (!query(db, sql.c_str(), printQueryResults, nullptr)) {
    std::cerr << "查询数据失败: " << getLastError(db) << std::endl;
    destroyWrapper(db);
    dlclose(handle);
    return 1;
  }

  // 更新数据
  if (!update(db, table_name.c_str(), "age = 26", "name = '张三'")) {
    std::cerr << "更新数据失败: " << getLastError(db) << std::endl;
    destroyWrapper(db);
    dlclose(handle);
    return 1;
  }
  std::cout << "更新数据成功" << std::endl;

  // 再次查询数据
  std::cout << "更新后查询所有用户:" << std::endl;
  if (!query(db, sql.c_str(), printQueryResults, nullptr)) {
    std::cerr << "查询数据失败: " << getLastError(db) << std::endl;
    destroyWrapper(db);
    dlclose(handle);
    return 1;
  }

  // 删除数据
  if (!remove(db, table_name.c_str(), "name = '李四'")) {
    std::cerr << "删除数据失败: " << getLastError(db) << std::endl;
    destroyWrapper(db);
    dlclose(handle);
    return 1;
  }
  std::cout << "删除数据成功" << std::endl;

  // 最后查询数据
  std::cout << "删除后查询所有用户:" << std::endl;
  if (!query(db, sql.c_str(), printQueryResults, nullptr)) {
    std::cerr << "查询数据失败: " << getLastError(db) << std::endl;
    destroyWrapper(db);
    dlclose(handle);
    return 1;
  }

  // 清理资源
  destroyWrapper(db);
  dlclose(handle);

  return 0;
}
