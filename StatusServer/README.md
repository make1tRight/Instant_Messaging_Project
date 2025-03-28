# StatusServer
负责负载均衡, 通过轮询机制查询负载更低的聊天服务与客户端简历连接
### StatusServer
#### RunServer
1. grpc服务器启动逻辑

## 配置管理层
### ConfigMgr
1. 将配置信息统一写在config.ini中
2. 配置管理类统一读取并设置对应参数
3. 配置有全局不变性, 实现为单例模式
4. 已加入配置信息(config.ini)
    1. 网关服务Port
    2. 验证服务IP与Port
    3. Redis的IP, Port和Passwd

## 数据访问层
### RedisMgr
1. 封装RedisConnPool用于管理与redis的连接
    1. redis连接基于TCP/IP, 每次创建连接都需要握手+AUTH
    2. 通过预先启动多个连接的方式, 建立好与redis的连接, 加快业务处理效率
    3. Defer机制保证程序运行过程中可正确归还Redis连接到池中
2. 已实现Redis指令
    1. `Connect`
    2. `Auth`
    3. `Get`和`HGet`
    4. `Set`和`HSet`
    5. `LPush`和`RPush`
    6. `LPop`和`RPop`
    7. `Del`
    8. `ExistsKey`
3. 实现redis功能测试函数`void TestRedis()`于main.cpp中

### MysqlMgr
1. 封装Dao层用于进行CRUD操作以及脚本运行
    1. 封装Dao层可在后期代码维护的时候将SQL换成其他数据库
    2. Dao允许多个服务模块调用相同的数据访问接口
2. 封装MySQL连接池管理MySQL连接
3. MysqlMgr内部调用Dao层
4. 已实现MySQL指令
    1. `CALL reg_user(?,?,?,@result)` 执行reg_user.sql脚本用于注册新用户
    2. `SELECT email FROM user WHERE name = ?` 查找用户邮箱
    3. `UPDATE user SET pwd = ? WHERE name = ?`更新密码

## gRPC通信
### StatusServiceImpl
1. StatusServiceImpl管理多个聊天服务
2. 通过轮询机制获取ChatServer连接
3. 重写proto中定义的GetChatServer函数, 返回客户端用于建立连接的信息
    1. 生成uuid作为token
    2. 返回聊天服务的ip+port|token



# 使用方法
```bash
# 1. 编译
cd StatusServer
sh ./build

# 2. 启动
# 直接启动current_path不对读不到config.ini
cd build
./StatusServer
```
# Debug