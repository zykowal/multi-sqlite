set(LIB_NAME sqlite_wrapper)

set(LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/sqlite_wrapper.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/sqlite_wrapper.h"
)

# 创建动态链接库
add_library(${LIB_NAME} SHARED ${LIB_SOURCES})

# 设置库的版本信息
set_target_properties(${LIB_NAME} PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    DEFINE_SYMBOL SQLITE_WRAPPER_EXPORTS
)

target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${LIB_NAME} PUBLIC sqlite3)
