#include "multi_connection_order_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

std::once_flag MultiConnectionOrderManager::initialized_;
std::unique_ptr<MultiConnectionOrderManager> MultiConnectionOrderManager::instance_;

MultiConnectionOrderManager& MultiConnectionOrderManager::getInstance() {
    std::call_once(initialized_, []() {
        instance_ = std::unique_ptr<MultiConnectionOrderManager>(new MultiConnectionOrderManager());
    });
    return *instance_;
}

MultiConnectionOrderManager::MultiConnectionOrderManager() 
    : db_connection_(MultiConnectionDatabaseManager::getInstance().getConnection(MultiConnectionDatabaseManager::TableType::ORDERS)),
      insert_stmt_(nullptr), select_all_stmt_(nullptr), 
      select_by_user_id_stmt_(nullptr), select_by_status_stmt_(nullptr),
      select_by_id_stmt_(nullptr), update_status_stmt_(nullptr),
      update_amount_stmt_(nullptr), delete_stmt_(nullptr),
      total_amount_by_user_stmt_(nullptr), count_by_status_stmt_(nullptr) {
    prepareStatements();
}

MultiConnectionOrderManager::~MultiConnectionOrderManager() {
    finalizeStatements();
}

void MultiConnectionOrderManager::prepareStatements() {
    auto db = db_connection_.get();
    
    // 准备插入语句
    const char* insert_sql = "INSERT INTO orders (user_id, total_amount, status) VALUES (?, ?, ?)";
    sqlite3_prepare_v2(db, insert_sql, -1, &insert_stmt_, nullptr);
    
    // 准备查询所有订单语句
    const char* select_all_sql = "SELECT id, user_id, total_amount, status, created_at, updated_at FROM orders ORDER BY created_at DESC";
    sqlite3_prepare_v2(db, select_all_sql, -1, &select_all_stmt_, nullptr);
    
    // 准备根据用户ID查询语句
    const char* select_by_user_id_sql = "SELECT id, user_id, total_amount, status, created_at, updated_at FROM orders WHERE user_id = ? ORDER BY created_at DESC";
    sqlite3_prepare_v2(db, select_by_user_id_sql, -1, &select_by_user_id_stmt_, nullptr);
    
    // 准备根据状态查询语句
    const char* select_by_status_sql = "SELECT id, user_id, total_amount, status, created_at, updated_at FROM orders WHERE status = ? ORDER BY created_at DESC";
    sqlite3_prepare_v2(db, select_by_status_sql, -1, &select_by_status_stmt_, nullptr);
    
    // 准备根据ID查询语句
    const char* select_by_id_sql = "SELECT id, user_id, total_amount, status, created_at, updated_at FROM orders WHERE id = ?";
    sqlite3_prepare_v2(db, select_by_id_sql, -1, &select_by_id_stmt_, nullptr);
    
    // 准备更新状态语句
    const char* update_status_sql = "UPDATE orders SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_prepare_v2(db, update_status_sql, -1, &update_status_stmt_, nullptr);
    
    // 准备更新金额语句
    const char* update_amount_sql = "UPDATE orders SET total_amount = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_prepare_v2(db, update_amount_sql, -1, &update_amount_stmt_, nullptr);
    
    // 准备删除语句
    const char* delete_sql = "DELETE FROM orders WHERE id = ?";
    sqlite3_prepare_v2(db, delete_sql, -1, &delete_stmt_, nullptr);
    
    // 准备统计用户总金额语句
    const char* total_amount_sql = "SELECT COALESCE(SUM(total_amount), 0) FROM orders WHERE user_id = ?";
    sqlite3_prepare_v2(db, total_amount_sql, -1, &total_amount_by_user_stmt_, nullptr);
    
    // 准备统计状态数量语句
    const char* count_by_status_sql = "SELECT COUNT(*) FROM orders WHERE status = ?";
    sqlite3_prepare_v2(db, count_by_status_sql, -1, &count_by_status_stmt_, nullptr);
}

void MultiConnectionOrderManager::finalizeStatements() {
    if (insert_stmt_) sqlite3_finalize(insert_stmt_);
    if (select_all_stmt_) sqlite3_finalize(select_all_stmt_);
    if (select_by_user_id_stmt_) sqlite3_finalize(select_by_user_id_stmt_);
    if (select_by_status_stmt_) sqlite3_finalize(select_by_status_stmt_);
    if (select_by_id_stmt_) sqlite3_finalize(select_by_id_stmt_);
    if (update_status_stmt_) sqlite3_finalize(update_status_stmt_);
    if (update_amount_stmt_) sqlite3_finalize(update_amount_stmt_);
    if (delete_stmt_) sqlite3_finalize(delete_stmt_);
    if (total_amount_by_user_stmt_) sqlite3_finalize(total_amount_by_user_stmt_);
    if (count_by_status_stmt_) sqlite3_finalize(count_by_status_stmt_);
}

bool MultiConnectionOrderManager::createOrder(int user_id, double total_amount, const std::string& status) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(insert_stmt_);
    sqlite3_bind_int(insert_stmt_, 1, user_id);
    sqlite3_bind_double(insert_stmt_, 2, total_amount);
    sqlite3_bind_text(insert_stmt_, 3, status.c_str(), -1, SQLITE_STATIC);
    
    int result = sqlite3_step(insert_stmt_);
    return result == SQLITE_DONE;
}

std::vector<Order> MultiConnectionOrderManager::getAllOrders() {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    std::vector<Order> orders;
    
    sqlite3_reset(select_all_stmt_);
    
    while (sqlite3_step(select_all_stmt_) == SQLITE_ROW) {
        Order order;
        order.id = sqlite3_column_int(select_all_stmt_, 0);
        order.user_id = sqlite3_column_int(select_all_stmt_, 1);
        order.total_amount = sqlite3_column_double(select_all_stmt_, 2);
        order.status = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 3));
        order.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 4));
        order.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 5));
        orders.push_back(order);
    }
    
    return orders;
}

std::vector<Order> MultiConnectionOrderManager::getOrdersByUserId(int user_id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    std::vector<Order> orders;
    
    sqlite3_reset(select_by_user_id_stmt_);
    sqlite3_bind_int(select_by_user_id_stmt_, 1, user_id);
    
    while (sqlite3_step(select_by_user_id_stmt_) == SQLITE_ROW) {
        Order order;
        order.id = sqlite3_column_int(select_by_user_id_stmt_, 0);
        order.user_id = sqlite3_column_int(select_by_user_id_stmt_, 1);
        order.total_amount = sqlite3_column_double(select_by_user_id_stmt_, 2);
        order.status = reinterpret_cast<const char*>(sqlite3_column_text(select_by_user_id_stmt_, 3));
        order.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_user_id_stmt_, 4));
        order.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_user_id_stmt_, 5));
        orders.push_back(order);
    }
    
    return orders;
}

std::vector<Order> MultiConnectionOrderManager::getOrdersByStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    std::vector<Order> orders;
    
    sqlite3_reset(select_by_status_stmt_);
    sqlite3_bind_text(select_by_status_stmt_, 1, status.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(select_by_status_stmt_) == SQLITE_ROW) {
        Order order;
        order.id = sqlite3_column_int(select_by_status_stmt_, 0);
        order.user_id = sqlite3_column_int(select_by_status_stmt_, 1);
        order.total_amount = sqlite3_column_double(select_by_status_stmt_, 2);
        order.status = reinterpret_cast<const char*>(sqlite3_column_text(select_by_status_stmt_, 3));
        order.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_status_stmt_, 4));
        order.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_status_stmt_, 5));
        orders.push_back(order);
    }
    
    return orders;
}

Order MultiConnectionOrderManager::getOrderById(int id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    Order order = {0, 0, 0.0, "", "", ""};
    
    sqlite3_reset(select_by_id_stmt_);
    sqlite3_bind_int(select_by_id_stmt_, 1, id);
    
    if (sqlite3_step(select_by_id_stmt_) == SQLITE_ROW) {
        order.id = sqlite3_column_int(select_by_id_stmt_, 0);
        order.user_id = sqlite3_column_int(select_by_id_stmt_, 1);
        order.total_amount = sqlite3_column_double(select_by_id_stmt_, 2);
        order.status = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 3));
        order.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 4));
        order.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 5));
    }
    
    return order;
}

bool MultiConnectionOrderManager::updateOrderStatus(int id, const std::string& status) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(update_status_stmt_);
    sqlite3_bind_text(update_status_stmt_, 1, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(update_status_stmt_, 2, id);
    
    int result = sqlite3_step(update_status_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool MultiConnectionOrderManager::updateOrderAmount(int id, double total_amount) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(update_amount_stmt_);
    sqlite3_bind_double(update_amount_stmt_, 1, total_amount);
    sqlite3_bind_int(update_amount_stmt_, 2, id);
    
    int result = sqlite3_step(update_amount_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool MultiConnectionOrderManager::deleteOrder(int id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(delete_stmt_);
    sqlite3_bind_int(delete_stmt_, 1, id);
    
    int result = sqlite3_step(delete_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

double MultiConnectionOrderManager::getTotalAmountByUserId(int user_id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(total_amount_by_user_stmt_);
    sqlite3_bind_int(total_amount_by_user_stmt_, 1, user_id);
    
    if (sqlite3_step(total_amount_by_user_stmt_) == SQLITE_ROW) {
        return sqlite3_column_double(total_amount_by_user_stmt_, 0);
    }
    
    return 0.0;
}

int MultiConnectionOrderManager::getOrderCountByStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(count_by_status_stmt_);
    sqlite3_bind_text(count_by_status_stmt_, 1, status.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(count_by_status_stmt_) == SQLITE_ROW) {
        return sqlite3_column_int(count_by_status_stmt_, 0);
    }
    
    return 0;
}

bool MultiConnectionOrderManager::createOrdersTransaction(const std::vector<std::tuple<int, double, std::string>>& orders) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    auto db = db_connection_.get();
    
    // 开始事务
    char* error_msg = nullptr;
    int result = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        sqlite3_free(error_msg);
        return false;
    }
    
    // 批量插入订单
    for (const auto& order_tuple : orders) {
        sqlite3_reset(insert_stmt_);
        sqlite3_bind_int(insert_stmt_, 1, std::get<0>(order_tuple));
        sqlite3_bind_double(insert_stmt_, 2, std::get<1>(order_tuple));
        sqlite3_bind_text(insert_stmt_, 3, std::get<2>(order_tuple).c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(insert_stmt_) != SQLITE_DONE) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
    }
    
    // 提交事务
    result = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        sqlite3_free(error_msg);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    
    return true;
}

void MultiConnectionOrderManager::performanceTest(int thread_count, int operations_per_thread) {
    std::cout << "\n=== 订单管理器性能测试 ===" << std::endl;
    std::cout << "线程数: " << thread_count << ", 每线程操作数: " << operations_per_thread << std::endl;
    
    std::vector<std::thread> threads;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([this, i, operations_per_thread]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> user_dis(1, 100);
            std::uniform_real_distribution<> amount_dis(10.0, 1000.0);
            std::uniform_int_distribution<> delay_dis(1, 10);
            
            for (int j = 0; j < operations_per_thread; ++j) {
                int user_id = user_dis(gen);
                double amount = amount_dis(gen);
                
                if (createOrder(user_id, amount, "pending")) {
                    // 随机延迟模拟真实场景
                    std::this_thread::sleep_for(std::chrono::microseconds(delay_dis(gen)));
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
    
    // 统计结果
    auto orders = getAllOrders();
    
    std::cout << "订单管理器测试结果:" << std::endl;
    std::cout << "- 预期创建订单数: " << thread_count * operations_per_thread << std::endl;
    std::cout << "- 实际创建订单数: " << orders.size() << std::endl;
    std::cout << "- 耗时: " << duration.count() << " 毫秒" << std::endl;
    std::cout << "- 成功率: " << (100.0 * orders.size() / (thread_count * operations_per_thread)) << "%" << std::endl;
}
