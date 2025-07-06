#pragma once

#include "database_manager.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>

struct Product {
    int id;
    std::string name;
    std::string description;
    double price;
    int stock_quantity;
    std::string created_at;
    std::string updated_at;
};

class ProductManager {
public:
    static ProductManager& getInstance();
    ~ProductManager();
    
    // 产品操作
    bool createProduct(const std::string& name, const std::string& description, 
                      double price, int stock_quantity = 0);
    std::vector<Product> getAllProducts();
    std::vector<Product> getProductsByPriceRange(double min_price, double max_price);
    std::vector<Product> getProductsInStock();
    Product getProductById(int id);
    Product getProductByName(const std::string& name);
    bool updateProduct(int id, const std::string& name, const std::string& description, 
                      double price, int stock_quantity);
    bool updateProductStock(int id, int stock_quantity);
    bool updateProductPrice(int id, double price);
    bool deleteProduct(int id);
    
    // 库存操作
    bool increaseStock(int id, int quantity);
    bool decreaseStock(int id, int quantity);
    int getStockQuantity(int id);
    
    // 批量操作
    bool createProductsTransaction(const std::vector<std::tuple<std::string, std::string, double, int>>& products);
    bool updateStockTransaction(const std::vector<std::pair<int, int>>& stock_updates);
    
private:
    ProductManager();
    ProductManager(const ProductManager&) = delete;
    ProductManager& operator=(const ProductManager&) = delete;
    
    void prepareStatements();
    void finalizeStatements();
    
    std::shared_ptr<sqlite3> db_connection_;
    std::mutex operation_mutex_;
    
    // 预编译的SQL语句
    sqlite3_stmt* insert_stmt_;
    sqlite3_stmt* select_all_stmt_;
    sqlite3_stmt* select_by_price_range_stmt_;
    sqlite3_stmt* select_in_stock_stmt_;
    sqlite3_stmt* select_by_id_stmt_;
    sqlite3_stmt* select_by_name_stmt_;
    sqlite3_stmt* update_stmt_;
    sqlite3_stmt* update_stock_stmt_;
    sqlite3_stmt* update_price_stmt_;
    sqlite3_stmt* delete_stmt_;
    sqlite3_stmt* increase_stock_stmt_;
    sqlite3_stmt* decrease_stock_stmt_;
    sqlite3_stmt* get_stock_stmt_;
    
    static std::once_flag initialized_;
    static std::unique_ptr<ProductManager> instance_;
};
