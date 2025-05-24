cmake_minimum_required(VERSION 3.22)

project(
    "Sqlite Demo"
    VERSION 1.0.0
    LANGUAGES C CXX)

# Global CMake variables are set here
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project/Library Names
set(EXECUTABLE_NAME "main")

include(FetchContent)
FetchContent_Declare(
        sqlite3
        GIT_REPOSITORY https://github.com/sqlite/sqlite
        GIT_TAG version-3.49.2
        GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(sqlite3)

add_subdirectory(src)
add_subdirectory(app)
