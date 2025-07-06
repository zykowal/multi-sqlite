cmake_minimum_required(VERSION 3.10)
project(SQLiteDemo)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找SQLite3库
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# 添加编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 包含头文件目录
include_directories(${SQLITE3_INCLUDE_DIRS})

# 添加源文件
set(SOURCES
    main.cpp
    database_manager.cpp
    user_manager.cpp
    order_manager.cpp
    product_manager.cpp
)

# 创建可执行文件
add_executable(sqlite_demo ${SOURCES})

# 链接库
target_link_libraries(sqlite_demo ${SQLITE3_LIBRARIES} pthread)

# 设置链接目录
target_link_directories(sqlite_demo PRIVATE ${SQLITE3_LIBRARY_DIRS})

# 设置编译定义
target_compile_definitions(sqlite_demo PRIVATE ${SQLITE3_CFLAGS_OTHER})
