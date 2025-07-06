#include <sqlite3.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <memory>

// 演示不同线程模式的区别
class ThreadModeDemo {
public:
    // FULLMUTEX模式 - 线程安全
    static void demonstrateFullMutex() {
        std::cout << "\n=== SQLITE_OPEN_FULLMUTEX 演示 ===" << std::endl;
        
        sqlite3* db = nullptr;
        int result = sqlite3_open_v2(
            "fullmutex_test.db",
            &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
            nullptr
        );
        
        if (result != SQLITE_OK) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return;
        }
        
        // 创建测试表
        const char* create_table = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, value TEXT)";
        sqlite3_exec(db, create_table, nullptr, nullptr, nullptr);
        
        // 使用shared_ptr管理数据库连接
        std::shared_ptr<sqlite3> shared_db(db, [](sqlite3* db) {
            sqlite3_close_v2(db);
        });
        
        // 启动多个线程同时操作同一个数据库连接
        std::vector<std::thread> threads;
        const int thread_count = 5;
        const int operations_per_thread = 100;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([shared_db, i, operations_per_thread]() {
                for (int j = 0; j < operations_per_thread; ++j) {
                    std::string sql = "INSERT INTO test_table (value) VALUES ('thread_" + 
                                     std::to_string(i) + "_op_" + std::to_string(j) + "')";
                    
                    char* error_msg = nullptr;
                    int result = sqlite3_exec(shared_db.get(), sql.c_str(), nullptr, nullptr, &error_msg);
                    
                    if (result != SQLITE_OK) {
                        std::cerr << "插入失败: " << error_msg << std::endl;
                        sqlite3_free(error_msg);
                    }
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // 统计插入的记录数
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(shared_db.get(), "SELECT COUNT(*) FROM test_table", -1, &stmt, nullptr);
        
        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        
        std::cout << "FULLMUTEX模式结果:" << std::endl;
        std::cout << "- 预期插入记录数: " << thread_count * operations_per_thread << std::endl;
        std::cout << "- 实际插入记录数: " << count << std::endl;
        std::cout << "- 耗时: " << duration.count() << " 毫秒" << std::endl;
        std::cout << "- 数据完整性: " << (count == thread_count * operations_per_thread ? "✓ 完整" : "✗ 丢失") << std::endl;
    }
    
    // NOMUTEX模式 - 非线程安全（危险演示）
    static void demonstrateNoMutex() {
        std::cout << "\n=== SQLITE_OPEN_NOMUTEX 演示 ===" << std::endl;
        std::cout << "警告: 这个演示可能会导致数据损坏或程序崩溃！" << std::endl;
        
        sqlite3* db = nullptr;
        int result = sqlite3_open_v2(
            "nomutex_test.db",
            &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
            nullptr
        );
        
        if (result != SQLITE_OK) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return;
        }
        
        // 创建测试表
        const char* create_table = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, value TEXT)";
        sqlite3_exec(db, create_table, nullptr, nullptr, nullptr);
        
        // 使用shared_ptr管理数据库连接
        std::shared_ptr<sqlite3> shared_db(db, [](sqlite3* db) {
            sqlite3_close_v2(db);
        });
        
        // 启动多个线程同时操作同一个数据库连接（危险！）
        std::vector<std::thread> threads;
        const int thread_count = 3; // 减少线程数避免过度损坏
        const int operations_per_thread = 50;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([shared_db, i, operations_per_thread]() {
                for (int j = 0; j < operations_per_thread; ++j) {
                    std::string sql = "INSERT INTO test_table (value) VALUES ('thread_" + 
                                     std::to_string(i) + "_op_" + std::to_string(j) + "')";
                    
                    char* error_msg = nullptr;
                    int result = sqlite3_exec(shared_db.get(), sql.c_str(), nullptr, nullptr, &error_msg);
                    
                    if (result != SQLITE_OK) {
                        // 在NOMUTEX模式下，多线程访问可能导致各种错误
                        std::cerr << "线程 " << i << " 操作 " << j << " 失败: " << error_msg << std::endl;
                        sqlite3_free(error_msg);
                    }
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // 统计插入的记录数
        sqlite3_stmt* stmt = nullptr;
        int prepare_result = sqlite3_prepare_v2(shared_db.get(), "SELECT COUNT(*) FROM test_table", -1, &stmt, nullptr);
        
        int count = 0;
        if (prepare_result == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        if (stmt) sqlite3_finalize(stmt);
        
        std::cout << "NOMUTEX模式结果:" << std::endl;
        std::cout << "- 预期插入记录数: " << thread_count * operations_per_thread << std::endl;
        std::cout << "- 实际插入记录数: " << count << std::endl;
        std::cout << "- 耗时: " << duration.count() << " 毫秒" << std::endl;
        std::cout << "- 数据完整性: " << (count == thread_count * operations_per_thread ? "✓ 完整" : "✗ 丢失/损坏") << std::endl;
    }
    
    // 正确的NOMUTEX使用方式 - 每个线程独立连接
    static void demonstrateNoMutexCorrect() {
        std::cout << "\n=== SQLITE_OPEN_NOMUTEX 正确使用方式 ===" << std::endl;
        
        // 启动多个线程，每个线程使用独立的数据库连接
        std::vector<std::thread> threads;
        const int thread_count = 5;
        const int operations_per_thread = 100;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([i, operations_per_thread]() {
                // 每个线程创建自己的数据库连接
                sqlite3* db = nullptr;
                int result = sqlite3_open_v2(
                    "nomutex_correct_test.db",
                    &db,
                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                    nullptr
                );
                
                if (result != SQLITE_OK) {
                    std::cerr << "线程 " << i << " 无法打开数据库" << std::endl;
                    return;
                }
                
                // 创建测试表
                const char* create_table = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, value TEXT)";
                sqlite3_exec(db, create_table, nullptr, nullptr, nullptr);
                
                // 执行操作
                for (int j = 0; j < operations_per_thread; ++j) {
                    std::string sql = "INSERT INTO test_table (value) VALUES ('thread_" + 
                                     std::to_string(i) + "_op_" + std::to_string(j) + "')";
                    
                    char* error_msg = nullptr;
                    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_msg);
                    
                    if (result != SQLITE_OK) {
                        std::cerr << "线程 " << i << " 插入失败: " << error_msg << std::endl;
                        sqlite3_free(error_msg);
                    }
                }
                
                sqlite3_close_v2(db);
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // 统计插入的记录数
        sqlite3* db = nullptr;
        sqlite3_open_v2("nomutex_correct_test.db", &db, SQLITE_OPEN_READONLY, nullptr);
        
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM test_table", -1, &stmt, nullptr);
        
        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        sqlite3_close_v2(db);
        
        std::cout << "NOMUTEX正确使用结果:" << std::endl;
        std::cout << "- 预期插入记录数: " << thread_count * operations_per_thread << std::endl;
        std::cout << "- 实际插入记录数: " << count << std::endl;
        std::cout << "- 耗时: " << duration.count() << " 毫秒" << std::endl;
        std::cout << "- 数据完整性: " << (count == thread_count * operations_per_thread ? "✓ 完整" : "✗ 丢失") << std::endl;
    }
};

int main() {
    std::cout << "SQLite 线程模式对比演示" << std::endl;
    
    // 清理之前的测试文件
    system("rm -f fullmutex_test.db* nomutex_test.db* nomutex_correct_test.db*");
    
    // 演示FULLMUTEX模式
    ThreadModeDemo::demonstrateFullMutex();
    
    // 演示NOMUTEX模式的危险用法
    ThreadModeDemo::demonstrateNoMutex();
    
    // 演示NOMUTEX模式的正确用法
    ThreadModeDemo::demonstrateNoMutexCorrect();
    
    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "1. FULLMUTEX: 线程安全，可以共享连接，有锁开销" << std::endl;
    std::cout << "2. NOMUTEX: 高性能，但需要每个线程独立连接" << std::endl;
    std::cout << "3. 在我们的高并发程序中选择FULLMUTEX是为了:" << std::endl;
    std::cout << "   - 简化线程管理（共享连接）" << std::endl;
    std::cout << "   - 保证数据安全" << std::endl;
    std::cout << "   - 配合预编译语句提高效率" << std::endl;
    
    return 0;
}
