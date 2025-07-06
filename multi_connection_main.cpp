#include "multi_connection_database_manager.h"
#include "multi_connection_user_manager.h"
#include "multi_connection_order_manager.h"
#include "multi_connection_product_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

void demonstrateBasicOperations() {
    std::cout << "\n=== 多连接基本操作演示 ===" << std::endl;
    
    auto& user_manager = MultiConnectionUserManager::getInstance();
    auto& order_manager = MultiConnectionOrderManager::getInstance();
    auto& product_manager = MultiConnectionProductManager::getInstance();
    
    // 创建一些基础数据
    user_manager.createUser("张三", "zhangsan@mc.com");
    user_manager.createUser("李四", "lisi@mc.com");
    user_manager.createUser("王五", "wangwu@mc.com");
    
    product_manager.createProduct("笔记本电脑", "高性能笔记本电脑", 5999.99, 10);
    product_manager.createProduct("无线鼠标", "蓝牙无线鼠标", 99.99, 50);
    product_manager.createProduct("机械键盘", "RGB机械键盘", 299.99, 20);
    
    order_manager.createOrder(1, 5999.99, "pending");
    order_manager.createOrder(2, 399.98, "completed");
    order_manager.createOrder(1, 99.99, "shipped");
    
    std::cout << "基础数据创建完成" << std::endl;
}

void demonstrateDistributedTransaction() {
    std::cout << "\n=== 分布式事务演示 ===" << std::endl;
    
    try {
        // 使用RAII管理分布式事务
        DistributedTransaction transaction;
        
        auto& user_manager = MultiConnectionUserManager::getInstance();
        auto& order_manager = MultiConnectionOrderManager::getInstance();
        auto& product_manager = MultiConnectionProductManager::getInstance();
        
        // 在事务中执行跨表操作
        bool user_created = user_manager.createUser("事务用户", "transaction@mc.com");
        bool product_created = product_manager.createProduct("事务产品", "事务测试产品", 199.99, 5);
        bool order_created = order_manager.createOrder(1, 199.99, "pending");
        
        if (user_created && product_created && order_created) {
            if (transaction.commit()) {
                std::cout << "分布式事务提交成功" << std::endl;
            } else {
                std::cout << "分布式事务提交失败" << std::endl;
            }
        } else {
            transaction.rollback();
            std::cout << "操作失败，事务已回滚" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "分布式事务异常: " << e.what() << std::endl;
    }
}

void demonstrateParallelPerformance() {
    std::cout << "\n=== 并行性能测试 ===" << std::endl;
    
    const int thread_count = 3;
    const int operations_per_thread = 50;
    
    std::vector<std::thread> test_threads;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 同时启动三个管理器的性能测试
    test_threads.emplace_back([thread_count, operations_per_thread]() {
        MultiConnectionUserManager::getInstance().performanceTest(thread_count, operations_per_thread);
    });
    
    test_threads.emplace_back([thread_count, operations_per_thread]() {
        MultiConnectionOrderManager::getInstance().performanceTest(thread_count, operations_per_thread);
    });
    
    test_threads.emplace_back([thread_count, operations_per_thread]() {
        MultiConnectionProductManager::getInstance().performanceTest(thread_count, operations_per_thread);
    });
    
    // 等待所有测试完成
    for (auto& thread : test_threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n=== 并行性能测试总结 ===" << std::endl;
    std::cout << "总耗时: " << total_duration.count() << " 毫秒" << std::endl;
    std::cout << "总操作数: " << 3 * thread_count * operations_per_thread << " 个操作" << std::endl;
    std::cout << "平均吞吐量: " << (3.0 * thread_count * operations_per_thread * 1000 / total_duration.count()) << " 操作/秒" << std::endl;
}

void demonstrateResourceUsage() {
    std::cout << "\n=== 资源使用情况 ===" << std::endl;
    
    auto& user_manager = MultiConnectionUserManager::getInstance();
    auto& order_manager = MultiConnectionOrderManager::getInstance();
    auto& product_manager = MultiConnectionProductManager::getInstance();
    
    // 统计各表的数据量
    auto users = user_manager.getAllUsers();
    auto orders = order_manager.getAllOrders();
    auto products = product_manager.getAllProducts();
    
    std::cout << "数据库连接数: 3 个独立连接" << std::endl;
    std::cout << "用户表记录数: " << users.size() << std::endl;
    std::cout << "订单表记录数: " << orders.size() << std::endl;
    std::cout << "产品表记录数: " << products.size() << std::endl;
    std::cout << "总记录数: " << (users.size() + orders.size() + products.size()) << std::endl;
}

int main() {
    try {
        std::cout << "SQLite 多连接高并发数据库程序启动" << std::endl;
        std::cout << "架构: NOMUTEX模式 + 每表独立连接" << std::endl;
        
        // 初始化数据库（会自动创建3个独立的数据库文件）
        MultiConnectionDatabaseManager::getInstance();
        
        // 演示基本操作
        demonstrateBasicOperations();
        
        // 演示分布式事务
        demonstrateDistributedTransaction();
        
        // 演示并行性能
        demonstrateParallelPerformance();
        
        // 显示资源使用情况
        demonstrateResourceUsage();
        
        std::cout << "\n=== 多连接架构优势 ===" << std::endl;
        std::cout << "✓ 真正的并行操作：不同表的操作完全独立" << std::endl;
        std::cout << "✓ 无锁竞争：NOMUTEX模式获得最佳性能" << std::endl;
        std::cout << "✓ 故障隔离：单个连接问题不影响其他表" << std::endl;
        std::cout << "✓ 扩展性好：可以轻松添加更多表" << std::endl;
        
        std::cout << "\n=== 需要注意的挑战 ===" << std::endl;
        std::cout << "⚠ 跨表事务复杂：需要分布式事务管理" << std::endl;
        std::cout << "⚠ 外键约束：跨数据库的外键无法直接实现" << std::endl;
        std::cout << "⚠ 资源消耗：3个连接 + 3套预编译语句" << std::endl;
        std::cout << "⚠ 一致性保证：需要应用层确保数据一致性" << std::endl;
        
        std::cout << "\n程序执行完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "程序执行出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
