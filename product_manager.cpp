#include "product_manager.h"
#include <iostream>

std::once_flag ProductManager::initialized_;
std::unique_ptr<ProductManager> ProductManager::instance_;

ProductManager& ProductManager::getInstance() {
    std::call_once(initialized_, []() {
        instance_ = std::unique_ptr<ProductManager>(new ProductManager());
    });
    return *instance_;
}

ProductManager::ProductManager() 
    : db_connection_(DatabaseManager::getInstance().getConnection()),
      insert_stmt_(nullptr), select_all_stmt_(nullptr), 
      select_by_price_range_stmt_(nullptr), select_in_stock_stmt_(nullptr),
      select_by_id_stmt_(nullptr), select_by_name_stmt_(nullptr),
      update_stmt_(nullptr), update_stock_stmt_(nullptr),
      update_price_stmt_(nullptr), delete_stmt_(nullptr),
      increase_stock_stmt_(nullptr), decrease_stock_stmt_(nullptr),
      get_stock_stmt_(nullptr) {
    prepareStatements();
}

ProductManager::~ProductManager() {
    finalizeStatements();
}

void ProductManager::prepareStatements() {
    auto db = db_connection_.get();
    
    // 准备插入语句
    const char* insert_sql = "INSERT INTO products (name, description, price, stock_quantity) VALUES (?, ?, ?, ?)";
    sqlite3_prepare_v2(db, insert_sql, -1, &insert_stmt_, nullptr);
    
    // 准备查询所有产品语句
    const char* select_all_sql = "SELECT id, name, description, price, stock_quantity, created_at, updated_at FROM products ORDER BY name";
    sqlite3_prepare_v2(db, select_all_sql, -1, &select_all_stmt_, nullptr);
    
    // 准备价格范围查询语句
    const char* select_by_price_range_sql = "SELECT id, name, description, price, stock_quantity, created_at, updated_at FROM products WHERE price BETWEEN ? AND ? ORDER BY price";
    sqlite3_prepare_v2(db, select_by_price_range_sql, -1, &select_by_price_range_stmt_, nullptr);
    
    // 准备有库存产品查询语句
    const char* select_in_stock_sql = "SELECT id, name, description, price, stock_quantity, created_at, updated_at FROM products WHERE stock_quantity > 0 ORDER BY name";
    sqlite3_prepare_v2(db, select_in_stock_sql, -1, &select_in_stock_stmt_, nullptr);
    
    // 准备根据ID查询语句
    const char* select_by_id_sql = "SELECT id, name, description, price, stock_quantity, created_at, updated_at FROM products WHERE id = ?";
    sqlite3_prepare_v2(db, select_by_id_sql, -1, &select_by_id_stmt_, nullptr);
    
    // 准备根据名称查询语句
    const char* select_by_name_sql = "SELECT id, name, description, price, stock_quantity, created_at, updated_at FROM products WHERE name = ?";
    sqlite3_prepare_v2(db, select_by_name_sql, -1, &select_by_name_stmt_, nullptr);
    
    // 准备更新语句
    const char* update_sql = "UPDATE products SET name = ?, description = ?, price = ?, stock_quantity = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_prepare_v2(db, update_sql, -1, &update_stmt_, nullptr);
    
    // 准备更新库存语句
    const char* update_stock_sql = "UPDATE products SET stock_quantity = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_prepare_v2(db, update_stock_sql, -1, &update_stock_stmt_, nullptr);
    
    // 准备更新价格语句
    const char* update_price_sql = "UPDATE products SET price = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_prepare_v2(db, update_price_sql, -1, &update_price_stmt_, nullptr);
    
    // 准备删除语句
    const char* delete_sql = "DELETE FROM products WHERE id = ?";
    sqlite3_prepare_v2(db, delete_sql, -1, &delete_stmt_, nullptr);
    
    // 准备增加库存语句
    const char* increase_stock_sql = "UPDATE products SET stock_quantity = stock_quantity + ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_prepare_v2(db, increase_stock_sql, -1, &increase_stock_stmt_, nullptr);
    
    // 准备减少库存语句
    const char* decrease_stock_sql = "UPDATE products SET stock_quantity = stock_quantity - ?, updated_at = CURRENT_TIMESTAMP WHERE id = ? AND stock_quantity >= ?";
    sqlite3_prepare_v2(db, decrease_stock_sql, -1, &decrease_stock_stmt_, nullptr);
    
    // 准备获取库存语句
    const char* get_stock_sql = "SELECT stock_quantity FROM products WHERE id = ?";
    sqlite3_prepare_v2(db, get_stock_sql, -1, &get_stock_stmt_, nullptr);
}

void ProductManager::finalizeStatements() {
    if (insert_stmt_) sqlite3_finalize(insert_stmt_);
    if (select_all_stmt_) sqlite3_finalize(select_all_stmt_);
    if (select_by_price_range_stmt_) sqlite3_finalize(select_by_price_range_stmt_);
    if (select_in_stock_stmt_) sqlite3_finalize(select_in_stock_stmt_);
    if (select_by_id_stmt_) sqlite3_finalize(select_by_id_stmt_);
    if (select_by_name_stmt_) sqlite3_finalize(select_by_name_stmt_);
    if (update_stmt_) sqlite3_finalize(update_stmt_);
    if (update_stock_stmt_) sqlite3_finalize(update_stock_stmt_);
    if (update_price_stmt_) sqlite3_finalize(update_price_stmt_);
    if (delete_stmt_) sqlite3_finalize(delete_stmt_);
    if (increase_stock_stmt_) sqlite3_finalize(increase_stock_stmt_);
    if (decrease_stock_stmt_) sqlite3_finalize(decrease_stock_stmt_);
    if (get_stock_stmt_) sqlite3_finalize(get_stock_stmt_);
}

bool ProductManager::createProduct(const std::string& name, const std::string& description, 
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

std::vector<Product> ProductManager::getAllProducts() {
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

std::vector<Product> ProductManager::getProductsByPriceRange(double min_price, double max_price) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    std::vector<Product> products;
    
    sqlite3_reset(select_by_price_range_stmt_);
    sqlite3_bind_double(select_by_price_range_stmt_, 1, min_price);
    sqlite3_bind_double(select_by_price_range_stmt_, 2, max_price);
    
    while (sqlite3_step(select_by_price_range_stmt_) == SQLITE_ROW) {
        Product product;
        product.id = sqlite3_column_int(select_by_price_range_stmt_, 0);
        product.name = reinterpret_cast<const char*>(sqlite3_column_text(select_by_price_range_stmt_, 1));
        product.description = reinterpret_cast<const char*>(sqlite3_column_text(select_by_price_range_stmt_, 2));
        product.price = sqlite3_column_double(select_by_price_range_stmt_, 3);
        product.stock_quantity = sqlite3_column_int(select_by_price_range_stmt_, 4);
        product.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_price_range_stmt_, 5));
        product.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_price_range_stmt_, 6));
        products.push_back(product);
    }
    
    return products;
}

std::vector<Product> ProductManager::getProductsInStock() {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    std::vector<Product> products;
    
    sqlite3_reset(select_in_stock_stmt_);
    
    while (sqlite3_step(select_in_stock_stmt_) == SQLITE_ROW) {
        Product product;
        product.id = sqlite3_column_int(select_in_stock_stmt_, 0);
        product.name = reinterpret_cast<const char*>(sqlite3_column_text(select_in_stock_stmt_, 1));
        product.description = reinterpret_cast<const char*>(sqlite3_column_text(select_in_stock_stmt_, 2));
        product.price = sqlite3_column_double(select_in_stock_stmt_, 3);
        product.stock_quantity = sqlite3_column_int(select_in_stock_stmt_, 4);
        product.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_in_stock_stmt_, 5));
        product.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_in_stock_stmt_, 6));
        products.push_back(product);
    }
    
    return products;
}

Product ProductManager::getProductById(int id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    Product product = {0, "", "", 0.0, 0, "", ""};
    
    sqlite3_reset(select_by_id_stmt_);
    sqlite3_bind_int(select_by_id_stmt_, 1, id);
    
    if (sqlite3_step(select_by_id_stmt_) == SQLITE_ROW) {
        product.id = sqlite3_column_int(select_by_id_stmt_, 0);
        product.name = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 1));
        product.description = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 2));
        product.price = sqlite3_column_double(select_by_id_stmt_, 3);
        product.stock_quantity = sqlite3_column_int(select_by_id_stmt_, 4);
        product.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 5));
        product.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_id_stmt_, 6));
    }
    
    return product;
}

Product ProductManager::getProductByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    Product product = {0, "", "", 0.0, 0, "", ""};
    
    sqlite3_reset(select_by_name_stmt_);
    sqlite3_bind_text(select_by_name_stmt_, 1, name.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(select_by_name_stmt_) == SQLITE_ROW) {
        product.id = sqlite3_column_int(select_by_name_stmt_, 0);
        product.name = reinterpret_cast<const char*>(sqlite3_column_text(select_by_name_stmt_, 1));
        product.description = reinterpret_cast<const char*>(sqlite3_column_text(select_by_name_stmt_, 2));
        product.price = sqlite3_column_double(select_by_name_stmt_, 3);
        product.stock_quantity = sqlite3_column_int(select_by_name_stmt_, 4);
        product.created_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_name_stmt_, 5));
        product.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(select_by_name_stmt_, 6));
    }
    
    return product;
}

bool ProductManager::updateProduct(int id, const std::string& name, const std::string& description, 
                                  double price, int stock_quantity) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(update_stmt_);
    sqlite3_bind_text(update_stmt_, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(update_stmt_, 2, description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(update_stmt_, 3, price);
    sqlite3_bind_int(update_stmt_, 4, stock_quantity);
    sqlite3_bind_int(update_stmt_, 5, id);
    
    int result = sqlite3_step(update_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool ProductManager::updateProductStock(int id, int stock_quantity) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(update_stock_stmt_);
    sqlite3_bind_int(update_stock_stmt_, 1, stock_quantity);
    sqlite3_bind_int(update_stock_stmt_, 2, id);
    
    int result = sqlite3_step(update_stock_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool ProductManager::updateProductPrice(int id, double price) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(update_price_stmt_);
    sqlite3_bind_double(update_price_stmt_, 1, price);
    sqlite3_bind_int(update_price_stmt_, 2, id);
    
    int result = sqlite3_step(update_price_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool ProductManager::deleteProduct(int id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(delete_stmt_);
    sqlite3_bind_int(delete_stmt_, 1, id);
    
    int result = sqlite3_step(delete_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool ProductManager::increaseStock(int id, int quantity) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(increase_stock_stmt_);
    sqlite3_bind_int(increase_stock_stmt_, 1, quantity);
    sqlite3_bind_int(increase_stock_stmt_, 2, id);
    
    int result = sqlite3_step(increase_stock_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

bool ProductManager::decreaseStock(int id, int quantity) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(decrease_stock_stmt_);
    sqlite3_bind_int(decrease_stock_stmt_, 1, quantity);
    sqlite3_bind_int(decrease_stock_stmt_, 2, id);
    sqlite3_bind_int(decrease_stock_stmt_, 3, quantity);
    
    int result = sqlite3_step(decrease_stock_stmt_);
    return result == SQLITE_DONE && sqlite3_changes(db_connection_.get()) > 0;
}

int ProductManager::getStockQuantity(int id) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    
    sqlite3_reset(get_stock_stmt_);
    sqlite3_bind_int(get_stock_stmt_, 1, id);
    
    if (sqlite3_step(get_stock_stmt_) == SQLITE_ROW) {
        return sqlite3_column_int(get_stock_stmt_, 0);
    }
    
    return -1; // 产品不存在
}

bool ProductManager::createProductsTransaction(const std::vector<std::tuple<std::string, std::string, double, int>>& products) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    auto db = db_connection_.get();
    
    // 开始事务
    char* error_msg = nullptr;
    int result = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        sqlite3_free(error_msg);
        return false;
    }
    
    // 批量插入产品
    for (const auto& product_tuple : products) {
        sqlite3_reset(insert_stmt_);
        sqlite3_bind_text(insert_stmt_, 1, std::get<0>(product_tuple).c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt_, 2, std::get<1>(product_tuple).c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(insert_stmt_, 3, std::get<2>(product_tuple));
        sqlite3_bind_int(insert_stmt_, 4, std::get<3>(product_tuple));
        
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

bool ProductManager::updateStockTransaction(const std::vector<std::pair<int, int>>& stock_updates) {
    std::lock_guard<std::mutex> lock(operation_mutex_);
    auto db = db_connection_.get();
    
    // 开始事务
    char* error_msg = nullptr;
    int result = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &error_msg);
    if (result != SQLITE_OK) {
        sqlite3_free(error_msg);
        return false;
    }
    
    // 批量更新库存
    for (const auto& stock_pair : stock_updates) {
        sqlite3_reset(update_stock_stmt_);
        sqlite3_bind_int(update_stock_stmt_, 1, stock_pair.second);
        sqlite3_bind_int(update_stock_stmt_, 2, stock_pair.first);
        
        if (sqlite3_step(update_stock_stmt_) != SQLITE_DONE) {
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
