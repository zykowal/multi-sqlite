# SQLite 高并发数据库程序

这是一个使用C++11和SQLite实现的高并发数据库操作程序，采用单例模式设计，支持多线程安全操作。

## 项目特点

### 高并发设计
- **WAL模式**: 启用Write-Ahead Logging，支持多个读取者和一个写入者同时操作
- **连接池**: 使用shared_ptr管理数据库连接，确保线程安全
- **预编译语句**: 所有SQL语句预编译，提高执行效率
- **细粒度锁**: 每个管理器使用独立的mutex，减少锁竞争

### 架构设计
- **单例模式**: 数据库管理器和各表管理器都采用线程安全的单例模式
- **分层设计**: 数据库连接层、业务逻辑层分离
- **RAII**: 使用智能指针自动管理资源

### 性能优化
- **批量操作**: 支持事务批量插入，提高大量数据操作效率
- **索引优化**: 为常用查询字段创建索引
- **缓存配置**: 优化SQLite缓存和同步设置

## 文件结构

```
sqlite_demo/
├── database_manager.h/cpp     # 数据库连接管理器
├── user_manager.h/cpp         # 用户表管理器
├── order_manager.h/cpp        # 订单表管理器
├── product_manager.h/cpp      # 产品表管理器
├── main.cpp                   # 主程序和演示代码
├── CMakeLists.txt            # CMake构建文件
├── Makefile                  # Make构建文件
└── README.md                 # 项目说明
```

## 数据库表结构

### 用户表 (users)
- id: 主键，自增
- username: 用户名，唯一
- email: 邮箱，唯一
- created_at/updated_at: 时间戳

### 订单表 (orders)
- id: 主键，自增
- user_id: 用户ID，外键
- total_amount: 订单金额
- status: 订单状态
- created_at/updated_at: 时间戳

### 产品表 (products)
- id: 主键，自增
- name: 产品名称
- description: 产品描述
- price: 价格
- stock_quantity: 库存数量
- created_at/updated_at: 时间戳

## 编译和运行

### 使用Make编译
```bash
# 安装依赖（macOS）
make install-deps

# 编译
make

# 运行
make run

# 清理
make clean
```

### 使用CMake编译
```bash
mkdir build
cd build
cmake ..
make
./sqlite_demo
```

### 手动编译
```bash
g++ -std=c++11 -Wall -Wextra -O2 -pthread \
    main.cpp database_manager.cpp user_manager.cpp \
    order_manager.cpp product_manager.cpp \
    -lsqlite3 -o sqlite_demo
```

## 并发特性说明

### 数据库配置
- **journal_mode=WAL**: 启用WAL模式，支持并发读写
- **synchronous=NORMAL**: 平衡性能和数据安全
- **cache_size=10000**: 设置较大的缓存提高性能
- **temp_store=MEMORY**: 临时数据存储在内存中
- **busy_timeout=30000**: 设置30秒的忙等待超时

### 线程安全机制
- 每个管理器类都有独立的mutex保护
- 使用RAII和智能指针管理资源
- 预编译语句重用，减少SQL解析开销

### 并发测试
程序包含多线程并发测试，模拟真实的高并发场景：
- 多线程同时创建用户、订单、产品
- 批量操作事务处理
- 并发查询和更新操作

## 使用示例

```cpp
// 获取管理器实例
auto& user_manager = UserManager::getInstance();
auto& order_manager = OrderManager::getInstance();
auto& product_manager = ProductManager::getInstance();

// 创建用户
user_manager.createUser("张三", "zhangsan@example.com");

// 创建产品
product_manager.createProduct("笔记本电脑", "高性能笔记本", 5999.99, 10);

// 创建订单
order_manager.createOrder(1, 5999.99, "pending");

// 批量操作
std::vector<std::pair<std::string, std::string>> users = {
    {"用户1", "user1@example.com"},
    {"用户2", "user2@example.com"}
};
user_manager.createUsersTransaction(users);
```

## 性能建议

1. **读多写少场景**: WAL模式非常适合读多写少的应用
2. **批量操作**: 大量数据操作时使用事务批量处理
3. **连接复用**: 程序使用单一连接+预编译语句，避免连接开销
4. **索引优化**: 根据查询模式添加合适的索引

## 注意事项

- SQLite的并发能力有限，适合中小型应用
- WAL模式下会产生额外的.wal和.shm文件
- 长时间运行的事务可能影响并发性能
- 建议定期执行VACUUM操作优化数据库文件

## 扩展建议

1. **连接池**: 可以扩展为真正的连接池支持更高并发
2. **读写分离**: 可以实现读写分离提高性能
3. **缓存层**: 添加Redis等缓存层减少数据库压力
4. **监控**: 添加性能监控和日志记录
