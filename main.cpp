#include "database_manager.h"
#include "user_manager.h"
#include "order_manager.h"
#include "product_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>

// 并发测试函数
void concurrentUserOperations(int thread_id, int operations_count) {
    auto& user_manager = UserManager::getInstance();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (int i = 0; i < operations_count; ++i) {
        std::string username = "user_" + std::to_string(thread_id) + "_" + std::to_string(i);
        std::string email = username + "@example.com";
        
        if (user_manager.createUser(username, email)) {
            std::cout << "线程 " << thread_id << " 创建用户: " << username << std::endl;
        }
        
        // 随机延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen) % 10));
    }
}

void concurrentOrderOperations(int thread_id, int operations_count) {
    auto& order_manager = OrderManager::getInstance();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> user_dis(1, 10);
    std::uniform_real_distribution<> amount_dis(10.0, 1000.0);
    
    for (int i = 0; i < operations_count; ++i) {
        int user_id = user_dis(gen);
        double amount = amount_dis(gen);
        
        if (order_manager.createOrder(user_id, amount)) {
            std::cout << "线程 " << thread_id << " 创建订单: 用户ID=" << user_id 
                      << ", 金额=" << amount << std::endl;
        }
        
        // 随机延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(gen() % 10));
    }
}

void concurrentProductOperations(int thread_id, int operations_count) {
    auto& product_manager = ProductManager::getInstance();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dis(1.0, 100.0);
    std::uniform_int_distribution<> stock_dis(0, 100);
    
    for (int i = 0; i < operations_count; ++i) {
        std::string name = "产品_" + std::to_string(thread_id) + "_" + std::to_string(i);
        std::string description = "描述_" + name;
        double price = price_dis(gen);
        int stock = stock_dis(gen);
        
        if (product_manager.createProduct(name, description, price, stock)) {
            std::cout << "线程 " << thread_id << " 创建产品: " << name 
                      << ", 价格=" << price << ", 库存=" << stock << std::endl;
        }
        
        // 随机延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(gen() % 10));
    }
}

void demonstrateBasicOperations() {
    std::cout << "\n=== 基本操作演示 ===" << std::endl;
    
    auto& user_manager = UserManager::getInstance();
    auto& order_manager = OrderManager::getInstance();
    auto& product_manager = ProductManager::getInstance();
    
    // 创建一些基础数据
    user_manager.createUser("张三", "zhangsan@example.com");
    user_manager.createUser("李四", "lisi@example.com");
    user_manager.createUser("王五", "wangwu@example.com");
    
    product_manager.createProduct("笔记本电脑", "高性能笔记本电脑", 5999.99, 10);
    product_manager.createProduct("无线鼠标", "蓝牙无线鼠标", 99.99, 50);
    product_manager.createProduct("机械键盘", "RGB机械键盘", 299.99, 20);
    
    order_manager.createOrder(1, 5999.99, "pending");
    order_manager.createOrder(2, 399.98, "completed");
    order_manager.createOrder(1, 99.99, "shipped");
    
    // 查询数据
    std::cout << "\n用户列表:" << std::endl;
    auto users = user_manager.getAllUsers();
    for (const auto& user : users) {
        std::cout << "ID: " << user.id << ", 用户名: " << user.username 
                  << ", 邮箱: " << user.email << std::endl;
    }
    
    std::cout << "\n产品列表:" << std::endl;
    auto products = product_manager.getAllProducts();
    for (const auto& product : products) {
        std::cout << "ID: " << product.id << ", 名称: " << product.name 
                  << ", 价格: " << product.price << ", 库存: " << product.stock_quantity << std::endl;
    }
    
    std::cout << "\n订单列表:" << std::endl;
    auto orders = order_manager.getAllOrders();
    for (const auto& order : orders) {
        std::cout << "ID: " << order.id << ", 用户ID: " << order.user_id 
                  << ", 金额: " << order.total_amount << ", 状态: " << order.status << std::endl;
    }
}

void demonstrateConcurrentOperations() {
    std::cout << "\n=== 并发操作演示 ===" << std::endl;
    
    const int thread_count = 5;
    const int operations_per_thread = 10;
    std::vector<std::thread> threads;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 启动用户操作线程
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(concurrentUserOperations, i, operations_per_thread);
    }
    
    // 启动订单操作线程
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(concurrentOrderOperations, i + thread_count, operations_per_thread);
    }
    
    // 启动产品操作线程
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(concurrentProductOperations, i + thread_count * 2, operations_per_thread);
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n并发操作完成，耗时: " << duration.count() << " 毫秒" << std::endl;
}

void demonstrateBatchOperations() {
    std::cout << "\n=== 批量操作演示 ===" << std::endl;
    
    auto& user_manager = UserManager::getInstance();
    auto& order_manager = OrderManager::getInstance();
    auto& product_manager = ProductManager::getInstance();
    
    // 批量创建用户
    std::vector<std::pair<std::string, std::string>> batch_users = {
        {"批量用户1", "batch1@example.com"},
        {"批量用户2", "batch2@example.com"},
        {"批量用户3", "batch3@example.com"},
        {"批量用户4", "batch4@example.com"},
        {"批量用户5", "batch5@example.com"}
    };
    
    if (user_manager.createUsersTransaction(batch_users)) {
        std::cout << "批量创建用户成功" << std::endl;
    }
    
    // 批量创建产品
    std::vector<std::tuple<std::string, std::string, double, int>> batch_products = {
        std::make_tuple("批量产品1", "批量产品描述1", 19.99, 100),
        std::make_tuple("批量产品2", "批量产品描述2", 29.99, 200),
        std::make_tuple("批量产品3", "批量产品描述3", 39.99, 150),
        std::make_tuple("批量产品4", "批量产品描述4", 49.99, 80),
        std::make_tuple("批量产品5", "批量产品描述5", 59.99, 120)
    };
    
    if (product_manager.createProductsTransaction(batch_products)) {
        std::cout << "批量创建产品成功" << std::endl;
    }
    
    // 批量创建订单
    std::vector<std::tuple<int, double, std::string>> batch_orders = {
        std::make_tuple(1, 199.99, "pending"),
        std::make_tuple(2, 299.99, "processing"),
        std::make_tuple(3, 399.99, "shipped"),
        std::make_tuple(1, 499.99, "completed"),
        std::make_tuple(2, 599.99, "pending")
    };
    
    if (order_manager.createOrdersTransaction(batch_orders)) {
        std::cout << "批量创建订单成功" << std::endl;
    }
}

int main() {
    try {
        std::cout << "SQLite 高并发数据库程序启动" << std::endl;
        
        // 初始化数据库（单例模式确保只初始化一次）
        DatabaseManager::getInstance();
        
        // 演示基本操作
        demonstrateBasicOperations();
        
        // 演示批量操作
        demonstrateBatchOperations();
        
        // 演示并发操作
        demonstrateConcurrentOperations();
        
        // 最终统计
        std::cout << "\n=== 最终统计 ===" << std::endl;
        auto& user_manager = UserManager::getInstance();
        auto& order_manager = OrderManager::getInstance();
        auto& product_manager = ProductManager::getInstance();
        
        auto final_users = user_manager.getAllUsers();
        auto final_orders = order_manager.getAllOrders();
        auto final_products = product_manager.getAllProducts();
        
        std::cout << "总用户数: " << final_users.size() << std::endl;
        std::cout << "总订单数: " << final_orders.size() << std::endl;
        std::cout << "总产品数: " << final_products.size() << std::endl;
        
        // 统计不同状态的订单数量
        std::cout << "待处理订单数: " << order_manager.getOrderCountByStatus("pending") << std::endl;
        std::cout << "已完成订单数: " << order_manager.getOrderCountByStatus("completed") << std::endl;
        
        std::cout << "\n程序执行完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "程序执行出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
