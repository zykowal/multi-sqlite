#include <sqlite3.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <memory>
#include <mutex>

int main() {
    std::cout << "SQLite 线程模式安全对比演示\n" << std::endl;
    
    // 清理之前的测试文件
    system("rm -f test_*.db*");
    
    // 1. FULLMUTEX模式演示
    std::cout << "=== SQLITE_OPEN_FULLMUTEX 模式 ===" << std::endl;
    {
        sqlite3* db = nullptr;
        int result = sqlite3_open_v2(
            "test_fullmutex.db",
            &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
            nullptr
        );
        
        if (result == SQLITE_OK) {
            // 创建测试表
            sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, thread_id INTEGER, op_id INTEGER)", nullptr, nullptr, nullptr);
            
            std::shared_ptr<sqlite3> shared_db(db, [](sqlite3* db) {
                sqlite3_close_v2(db);
            });
            
            const int thread_count = 3;
            const int ops_per_thread = 50;
            std::vector<std::thread> threads;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // 多个线程共享同一个数据库连接
            for (int i = 0; i < thread_count; ++i) {
                threads.emplace_back([shared_db, i, ops_per_thread]() {
                    for (int j = 0; j < ops_per_thread; ++j) {
                        sqlite3_stmt* stmt = nullptr;
                        const char* sql = "INSERT INTO test (thread_id, op_id) VALUES (?, ?)";
                        
                        if (sqlite3_prepare_v2(shared_db.get(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
                            sqlite3_bind_int(stmt, 1, i);
                            sqlite3_bind_int(stmt, 2, j);
                            sqlite3_step(stmt);
                            sqlite3_finalize(stmt);
                        }
                    }
                });
            }
            
            for (auto& t : threads) {
                t.join();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // 统计结果
            sqlite3_stmt* stmt = nullptr;
            sqlite3_prepare_v2(shared_db.get(), "SELECT COUNT(*) FROM test", -1, &stmt, nullptr);
            int count = 0;
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
            
            std::cout << "✓ 共享连接模式" << std::endl;
            std::cout << "  预期记录数: " << thread_count * ops_per_thread << std::endl;
            std::cout << "  实际记录数: " << count << std::endl;
            std::cout << "  耗时: " << duration.count() << " ms" << std::endl;
            std::cout << "  数据完整性: " << (count == thread_count * ops_per_thread ? "✓" : "✗") << std::endl;
        }
    }
    
    // 2. NOMUTEX模式演示（正确用法）
    std::cout << "\n=== SQLITE_OPEN_NOMUTEX 模式（每线程独立连接）===" << std::endl;
    {
        const int thread_count = 3;
        const int ops_per_thread = 50;
        std::vector<std::thread> threads;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 每个线程使用独立的数据库连接
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([i, ops_per_thread]() {
                sqlite3* db = nullptr;
                int result = sqlite3_open_v2(
                    "test_nomutex.db",
                    &db,
                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                    nullptr
                );
                
                if (result == SQLITE_OK) {
                    // 创建测试表
                    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, thread_id INTEGER, op_id INTEGER)", nullptr, nullptr, nullptr);
                    
                    for (int j = 0; j < ops_per_thread; ++j) {
                        sqlite3_stmt* stmt = nullptr;
                        const char* sql = "INSERT INTO test (thread_id, op_id) VALUES (?, ?)";
                        
                        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                            sqlite3_bind_int(stmt, 1, i);
                            sqlite3_bind_int(stmt, 2, j);
                            sqlite3_step(stmt);
                            sqlite3_finalize(stmt);
                        }
                    }
                    
                    sqlite3_close_v2(db);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // 统计结果
        sqlite3* db = nullptr;
        sqlite3_open_v2("test_nomutex.db", &db, SQLITE_OPEN_READONLY, nullptr);
        
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM test", -1, &stmt, nullptr);
        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        sqlite3_close_v2(db);
        
        std::cout << "✓ 独立连接模式" << std::endl;
        std::cout << "  预期记录数: " << thread_count * ops_per_thread << std::endl;
        std::cout << "  实际记录数: " << count << std::endl;
        std::cout << "  耗时: " << duration.count() << " ms" << std::endl;
        std::cout << "  数据完整性: " << (count == thread_count * ops_per_thread ? "✓" : "✗") << std::endl;
    }
    
    std::cout << "\n=== 性能和使用场景分析 ===" << std::endl;
    std::cout << "\nFULLMUTEX 模式特点:" << std::endl;
    std::cout << "✓ 线程安全，可以共享数据库连接" << std::endl;
    std::cout << "✓ 简化资源管理" << std::endl;
    std::cout << "✓ 适合连接池设计" << std::endl;
    std::cout << "✗ 有互斥锁开销" << std::endl;
    std::cout << "✗ 可能存在锁竞争" << std::endl;
    
    std::cout << "\nNOMUTEX 模式特点:" << std::endl;
    std::cout << "✓ 无锁开销，性能最佳" << std::endl;
    std::cout << "✓ 无锁竞争" << std::endl;
    std::cout << "✗ 需要每个线程独立连接" << std::endl;
    std::cout << "✗ 资源管理复杂" << std::endl;
    std::cout << "✗ 连接数可能过多" << std::endl;
    
    std::cout << "\n在我们的高并发程序中选择FULLMUTEX的原因:" << std::endl;
    std::cout << "1. 使用单一共享连接，简化设计" << std::endl;
    std::cout << "2. 配合预编译语句，减少重复准备开销" << std::endl;
    std::cout << "3. 结合WAL模式，支持并发读写" << std::endl;
    std::cout << "4. 应用层已有mutex保护，双重保险" << std::endl;
    std::cout << "5. 适合中等并发量的应用场景" << std::endl;
    
    return 0;
}
