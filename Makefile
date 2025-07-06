# Makefile for SQLite Demo

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread
LIBS = -lsqlite3

# 源文件
SOURCES = main.cpp database_manager.cpp user_manager.cpp order_manager.cpp product_manager.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = sqlite_demo

# 默认目标
all: $(TARGET)

# 链接目标
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# 编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJECTS) $(TARGET) *.db *.db-wal *.db-shm

# 运行
run: $(TARGET)
	./$(TARGET)

# 调试版本
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# 安装依赖（macOS）
install-deps:
	brew install sqlite3

# 检查内存泄漏（需要valgrind）
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

.PHONY: all clean run debug install-deps memcheck
