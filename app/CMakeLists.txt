set(APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cc")

add_executable(${EXECUTABLE_NAME} ${APP_SOURCES})

# 链接动态库加载库 (dlopen, dlsym等)
if(APPLE)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE "-ldl")
elseif(UNIX)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE dl)
endif()

# 不再直接链接sqlite_wrapper库，因为我们将使用动态加载
