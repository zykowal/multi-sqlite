#pragma once

#include "database_manager.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>

struct Order {
    int id;
    int user_id;
    double total_amount;
    std::string status;
    std::string created_at;
    std::string updated_at;
};

class OrderManager {
public:
    static OrderManager& getInstance();
    ~OrderManager();
    
    // 订单操作
    bool createOrder(int user_id, double total_amount, const std::string& status = "pending");
    std::vector<Order> getAllOrders();
    std::vector<Order> getOrdersByUserId(int user_id);
    std::vector<Order> getOrdersByStatus(const std::string& status);
    Order getOrderById(int id);
    bool updateOrderStatus(int id, const std::string& status);
    bool updateOrderAmount(int id, double total_amount);
    bool deleteOrder(int id);
    
    // 统计操作
    double getTotalAmountByUserId(int user_id);
    int getOrderCountByStatus(const std::string& status);
    
    // 批量操作
    bool createOrdersTransaction(const std::vector<std::tuple<int, double, std::string>>& orders);
    
private:
    OrderManager();
    OrderManager(const OrderManager&) = delete;
    OrderManager& operator=(const OrderManager&) = delete;
    
    void prepareStatements();
    void finalizeStatements();
    
    std::shared_ptr<sqlite3> db_connection_;
    std::mutex operation_mutex_;
    
    // 预编译的SQL语句
    sqlite3_stmt* insert_stmt_;
    sqlite3_stmt* select_all_stmt_;
    sqlite3_stmt* select_by_user_id_stmt_;
    sqlite3_stmt* select_by_status_stmt_;
    sqlite3_stmt* select_by_id_stmt_;
    sqlite3_stmt* update_status_stmt_;
    sqlite3_stmt* update_amount_stmt_;
    sqlite3_stmt* delete_stmt_;
    sqlite3_stmt* total_amount_by_user_stmt_;
    sqlite3_stmt* count_by_status_stmt_;
    
    static std::once_flag initialized_;
    static std::unique_ptr<OrderManager> instance_;
};
