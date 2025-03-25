# GateServer
用于建立http短连接, 主要面向注册, 修改密码, 登录等功能
### GateServer
1. 服务器启动与优雅退出

## 网络层
### CServer
1. 负责监听新的客户连接+分发连接
2. 通过iocontext实现, iocontext底层调用epoll


### HttpConnection
1. 负责读写, 将结果提交至逻辑队列
2. 根据http请求Get/Post, 分发到逻辑系统中进行处理

### IOContextPool
IOContext连接池, 用于提升并发性能, 改善处理http连接的效率
1. 根据CPU核数封装线程数
    - 线程数少于CPU核数, 可能使CPU出现空闲的核心造成资源浪费
    - 线程数多于CPU核数, 可能出现多线程竞争1核心的情况, 多了时间片分配和线程上下文切换的开销
2. CServer从池中取出iocontext, 用完放回不释放
    - 避免在使用过程中iocontext因频繁构造和释放影响性能

## 逻辑层
### LogicSystem
1. 负责将逻辑队列中的消息取出, 根据消息id, 选择对应回调函数进行处理
2. 采用注册-处理机制来管理http连接请求, 不管请求类型怎么增加, 都不需要改变代码结构
3. unordered_map实现了O(1)查找时间复杂度
4. 主要逻辑说明:
    1. `/get_test` Get请求处理(get报文测试, 可删除)
    2. `/get_varifycode` Post请求处理(获取验证码请求)
    3. `/user_register` 用户注册(将用户信息加入到MySQL数据库中)
    4. `/reset_pwd` 修改密码逻辑(根据name找到pwd并进行update)

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
### VarifyGrpcClient
1. VarifyGrpcClient通过gRPC与验证服务进行通信(获取验证码)
2. message.proto定义了各服务通信所需要的方法(req returns rsp)


# 使用方法
```bash
# 1. 编译
cd GateServer
sh ./build

# 2. 启动
# 直接启动current_path不对读不到config.ini
cd build
./GateServer
```

# Debug
## `bad_weak_ptr` 错误 -> 继承忘记加public
1. 使用`shared_from_this()`方法的类
    - 必须使用`std::shared_ptr<T>`进行管理
    - 必须继承`public std::enable_shared_from_this<T>`
2. 以上两条如果缺少, 会出现`bad_weak_ptr`的exception
## gRPC的编译问题
1. 使用gRPC要包含Protobuf, utf8_range库
    1. gRPC依赖protobuf进行序列化
    2. protobuf需要utf8编码, 需要utf8_range库
## `http read error, code: Bad file descriptor`
1. HttpConnection进行读写的时候socket已经关闭
    - `async_accept`要从HttpConnection内部获取socket
    - 保证HttpConnection在进行读写处理的时候socket不会关闭
## reg_user运行错误返回`-1`
1. 捕捉并输出对应的错误信息
```sql
-- ...
    -- 声明变量来捕获错误信息
    DECLARE error_message VARCHAR(255);
    -- 如果在执行过程中遇到任何错误，则回滚事务
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        -- 捕获错误并设置错误信息
        GET DIAGNOSTICS CONDITION 1 error_message = MESSAGE_TEXT;
        SET result = -1;
        -- 输出错误信息
        SELECT CONCAT('Error occurred: ', error_message) AS ErrorMessage;
-- ...
```
2. `Illegal mix of collations`错误表示在执行查询时，字符集或排序规则 (collation) 的不一致导致操作失败。
```bash
mysql> CALL reg_user('xxxx' COLLATE utf8mb4_unicode_ci, 'xxxx@example.com' COLLATE utf8mb4_unicode_ci, 'mypassword' COLLATE utf8mb4_unicode_ci, @result);
+-----------------------------------------------------------------------------------------------------------------------------+
| ErrorMessage                                                                                                                |
+-----------------------------------------------------------------------------------------------------------------------------+
| Error occurred: Illegal mix of collations (utf8mb4_unicode_ci,IMPLICIT) and (utf8mb4_0900_ai_ci,IMPLICIT) for operation '=' |
+-----------------------------------------------------------------------------------------------------------------------------+
1 row in set (0.00 sec)
```
3. 修改表的字符集和排序规则(有时间找一下修改指令的做法, 随意调整表结构不妥)
```sql
ALTER TABLE `user` CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;
```