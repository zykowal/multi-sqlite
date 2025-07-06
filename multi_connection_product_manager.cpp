#include "multi_connection_product_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

std::once_flag MultiConnectionProductManager::initialized_;
std::unique_ptr<MultiConnectionProductManager> MultiConnectionProductManager::instance_;

MultiConnectionProductManager& MultiConnectionProductManager::getInstance() {
    std::call_once(initialized_, []() {
        instance_ = std::unique_ptr<MultiConnectionProductManager>(new MultiConnectionProductManager());
    });
    return *instance_;
}

MultiConnectionProductManager::MultiConnectionProductManager() 
    : db_connection_(MultiConnectionDatabaseManager::getInstance().getConnection(MultiConnectionDatabaseManager::TableType::PRODUCTS)),
      insert_stmt_(nullptr), select_all_stmt_(nullptr), 
      select_by_price_range_stmt_(nullptr), select_in_stock_stmt_(nullptr),
      select_by_id_stmt_(nullptr), select_by_name_stmt_(nullptr),
      update_stmt_(nullptr), update_stock_stmt_(nullptr),
      update_price_stmt_(nullptr), delete_stmt_(nullptr),
      increase_stock_stmt_(nullptr), decrease_stock_stmt_(nullptr),
      get_stock_stmt_(nullptr) {
    prepareStatements();
}

MultiConnectionProductManager::~MultiConnectionProductManager() {
    finalizeStatements();
}

void MultiConnectionProductManager::prepareStatements() {
    auto db = db_connection_.get();
    
    // 准备插入语句
    const char* insert_sql = "INSERT INTO products (name, description, price, stock_quantity) VALUES (?, ?, ?, ?)";
    sqlite3_prepare_v2(db, insert_sql, -1, &insert_stmt_, nullptr);
    
    // 准备查询所有产品语句
    const char* select_all_sql = "SELECT id, name, description, price, stock_quantity, created_at, updated_at FROM products ORDER BY name";
    sqlite3_prepare_v2(db, select_all_sql, -1, &select_all_stmt_, nullptr);
    
    // 其他语句准备（简化版本，只实现核心功能）
    const char* select_by_id_sql = "SELECT id, name, description, price, stock_quantity, created_at, updated_at FROM products WHERE id = ?";
    sqlite3_prepare_v2(db, select_by_id_sql, -1, &select_by_id_stmt_, nullptr);
    
    const char* update_stock_sql = "UPDATE products SET stock_quantity = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_prepare_v2(db, update_stock_sql, -1, &update_stock_stmt_, nullptr);
}

void MultiConnectionProductManager::finalizeStatements() {
    if (insert_stmt_) sqlite3_finalize(insert_stmt_);
    if (select_all_stmt_) sqlite3_finalize(select_all_stmt_);
    if (select_by_id_stmt_) sqlite3_finalize(select_by_id_stmt_);
    if (update_stock_stmt_) sqlite3_finalize(update_stock_stmt_);
    // 其他语句清理...
}

bool MultiConnectionProductManager::createProduct(const std::string& name, const std::string& description, 
                                                 double price, int stock_quantity) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(insert_stmt_);
    sqlite3_bind_text(insert_stmt_, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt_, 2, description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(insert_stmt_, 3, price);
    sqlite3_bind_int(insert_stmt_, 4, stock_quantity);
    
    int result = sqlite3_step(insert_stmt_);
    return result == SQLITE_DONE;
}

std::vector<Product> MultiConnectionProductManager::getAllProducts() {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    std::vector<Product> products;
    
    sqlite3_reset(select_all_stmt_);
    
    while (sqlite3_step(select_all_stmt_) == SQLITE_ROW) {
        Product product;
        product.id = sqlite3_column_int(select_all_stmt_, 0);
        product.name = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 1));
        product.description = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 2));
        product.price = sqlite3_column_double(select_all_stmt_, 3);
        product.stock_quantity = sqlite3_column_int(select_all_stmt_, 4);
        product.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 5));
        product.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_all_stmt_, 6));
        products.push_back(product);
    }
    
    return products;
}

// 简化实现其他方法
std::vector<Product> MultiConnectionProductManager::getProductsByPriceRange(double, double) { return {}; }
std::vector<Product> MultiConnectionProductManager::getProductsInStock() { return {}; }
Product MultiConnectionProductManager::getProductById(int) { return {0, "", "", 0.0, 0, "", ""}; }
Product MultiConnectionProductManager::getProductByName(const std::string&) { return {0, "", "", 0.0, 0, "", ""}; }
bool MultiConnectionProductManager::updateProduct(int, const std::string&, const std::string&, double, int) { return false; }
bool MultiConnectionProductManager::updateProductStock(int, int) { return false; }
bool MultiConnectionProductManager::updateProductPrice(int, double) { return false; }
bool MultiConnectionProductManager::deleteProduct(int) { return false; }
bool MultiConnectionProductManager::increaseStock(int, int) { return false; }
bool MultiConnectionProductManager::decreaseStock(int, int) { return false; }
int MultiConnectionProductManager::getStockQuantity(int) { return 0; }
bool MultiConnectionProductManager::createProductsTransaction(const std::vector<std::tuple<std::string, std::string, double, int>>&) { return false; }
bool MultiConnectionProductManager::updateStockTransaction(const std::vector<std::pair<int, int>>&) { return false; }

void MultiConnectionProductManager::performanceTest(int thread_count, int operations_per_thread) {
    std::cout << "\n=== 产品管理器性能测试 ===" << std::endl;
    std::cout << "线程数: " << thread_count << ", 每线程操作数: " << operations_per_thread << std::endl;
    
    std::vector<std::thread> threads;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([this, i, operations_per_thread]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> price_dis(1.0, 100.0);
            std::uniform_int_distribution<> stock_dis(0, 100);
            std::uniform_int_distribution<> delay_dis(1, 10);
            
            for (int j = 0; j < operations_per_thread; ++j) {
                std::string name = "mc_product_" + std::to_string(i) + "_" + std::to_string(j);
                std::string description = "描述_" + name;
                double price = price_dis(gen);
                int stock = stock_dis(gen);
                
                if (createProduct(name, description, price, stock)) {
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
    auto products = getAllProducts();
    int new_products = 0;
    for (const auto& product : products) {
        if (product.name.find("mc_product_") == 0) {
            new_products++;
        }
    }
    
    std::cout << "产品管理器测试结果:" << std::endl;
    std::cout << "- 预期创建产品数: " << thread_count * operations_per_thread << std::endl;
    std::cout << "- 实际创建产品数: " << new_products << std::endl;
    std::cout << "- 耗时: " << duration.count() << " 毫秒" << std::endl;
    std::cout << "- 成功率: " << (100.0 * new_products / (thread_count * operations_per_thread)) << "%" << std::endl;
}
