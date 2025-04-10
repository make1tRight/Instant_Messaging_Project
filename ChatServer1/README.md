# ChatServer1
用于建立http短连接, 主要面向注册, 修改密码, 登录等功能
### ChatServer
1. 服务器启动与优雅退出

## 网络层
### CServer
1. 负责监听新的客户连接+分发连接
2. 通过iocontext实现, iocontext底层调用epoll
3. `HandleAccept`处理接收下一个连接的时机, 注意和Http连接的`Start`直接递归调用的区别

### CSession
1. 会话管理类, 负责解析客户端发来的tlv格式报文 
2. 将解析完成的消息投入到消息队列中, 由逻辑系统进行处理
3. 负责将逻辑系统处理完毕的响应报文返回给客户端
4. tcp长连接实现的原理
    1. `Start`内部封装`AsyncReadHead`, `AsyncReadHead`内部封装`AsyncReadBody`
    2. `AsyncReadHead`和`AsyncReadBody`相互调用维持了持续的消息监听, 建立了长连接
    3. 当出现error时调用`Close`关闭socket, 调用`ClearSession`删除会话

### IOContextPool
IOContext连接池, 用于提升并发性能, 改善处理每个session的tcp连接效率
1. 根据CPU核数封装线程数
    - 线程数少于CPU核数, 可能使CPU出现空闲的核心造成资源浪费
    - 线程数多于CPU核数, 可能出现多线程竞争1核心的情况, 多了时间片分配和线程上下文切换开销
2. CServer从池中取出iocontext, 用完放回不释放
    - 避免在使用过程中iocontext因频繁构造和释放影响性能

## 逻辑层
### LogicSystem
1. 负责将逻辑队列中的消息取出, 根据消息id, 选择对应回调函数进行处理
2. 采用注册-处理机制来管理tcp连接发来的请求, 不管请求类型怎么增加, 都不需要改变代码结构
3. unordered_map实现了O(1)查找时间复杂度
4. 主要逻辑说明:
    1. `void LoginHandler` 处理用户登录逻辑
        1. 获取当前用户数据信息, 先查缓存, 缓存中没有才访问数据库
        2. 数据库中拉取好友申请列表
        3. 数据库中拉取联系人列表
        4. 更新缓存中服务活跃的用户数量
    2. `void AddFriendApply` 处理好友申请
        1. 如果接收方用户处于同一服务且在线则直接通过tcp发送请求
        2. 如果接收方用户处于不同服务则通过grpc通知对应服务
        3. 好友申请的持久化
    3. `void AuthFriendApply` 处理好友认证
        1. 发送给认证方客户端的回包
        2. 如果发出好友申请的用户处于同一服务且在线则直接通过tcp发送认证方的信息并提示认证通过
        3. 如果接收方用户处于不同服务则通过grpc通知对应服务
    4. `void DealChatTextMsg` 处理消息文本的发送
        1. 如果接收方用户处于同一服务且在线则直接通过tcp发送聊天消息
        2. 如果接收方用户处于不同服务则先通过grpc通知对应服务
        3. 将消息展示给自己, 用于调试
        4. 为了确保消息的唯一性, 客户端生成了uuid作为消息的id
5. 与网关服务逻辑层的区别
    1. 只有1个工作线程
        1. 避免竞态条件
        2. 严格按照消息到来的顺序进行逻辑处理
        3. 降低了线程切换的开销
    2. 配合消息队列异步处理消息, 具有一定的并发性能

## 配置管理层
### ConfigMgr
1. 将配置信息统一写在config.ini中
2. 配置管理类统一读取并设置对应参数
3. 配置有全局不变性, 实现为单例模式
4. 已加入配置信息(config.ini)
    1. 网关服务Port
    2. 验证服务IP与Port
    3. Redis的IP, Port和Passwd
    4. 状态服务的IP与Port
    5. 聊天服务
        1. 服务名用于缓存和服务发现
        2. IP和Port用于建立连接
        3. RPCPort用于多个聊天服务通信

## 数据访问层
### RedisMgr(直接复用GateServer)
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
    8. `HDel` 退出时删除对应的HASH数据, 避免状态服务发现无效的聊天服务
    9. `ExistsKey`

### MysqlMgr
1. 封装Dao层用于进行CRUD操作以及脚本运行
    1. 封装Dao层可在后期代码维护的时候将SQL换成其他数据库
    2. Dao允许多个服务模块调用相同的数据访问接口
2. 封装MySQL连接池管理MySQL连接
3. MysqlMgr内部调用Dao层
    1. 已实现MySQL指令
        1. `"SELECT * FROM friend WHERE self_id = ?"` 获取当前用户的所有好友信息
        2. `"SELECT apply.from_uid, apply.status, user.name, user.nick, user.sex FROM friend_apply AS apply JOIN user ON apply.from_uid = user.uid WHERE apply.to_uid = ? AND apply.id > ? ORDER BY apply.id ASC LIMIT ? "` 获取好友申请列表, 按friend_apply的id列升序排列
        3. `"INSERT INTO friend_apply (from_uid, to_uid) values (?, ?) ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"` 新增好友申请
        4. `"SELECT * FROM user WHERE uid = ?"` 通过uid获取用户信息
        5. `"SELECT * FROM user WHERE name = ?"` 通过name获取用户信息
        6. `INSERT IGNORE INTO friend(self_id, friend_id, back) VALUES (?, ?, ?)` 更新好友关系(一对好友关系用了两条记录)
5. 先查缓存Redis后查MySQL降低数据库压力

## gRPC通信
### ChatConnPool
实现`ChatConnPool` grpc连接池用于提升grpc连接的并发性能
1. 每个gRPC客户端都使用多个stub组织成的队列来管理
2. stub由同一个channel创建, 底层是同一个tcp连接(HTTP/2支持多路复用)
### ChatGrpcClient
聊天双方的gRPC接收端
1. `NotifyAddFriend` 通过gRPC发送"添加好友申请"到接收方所在服务
2. `NotifyAuthFriend` 通过gRPC发送"认证通过"到接收方所在服务
3. `NotifyTextChatMsg` 通过gRPC发送消息文本到接收方所在服务
### ChatServiceImpl
聊天双方的gRPC发送端
1. `NotifyAddFriend` 判断用户是否在内存中, 存在则通过tcp连接发送通知给新好友的客户端
2. `NotifyAuthFriend` 用户在线则调用session->Send通过tcp连接发送好友认证成功的通知给好友申请方
3. `NotifyTextChatMsg` 用户在线则调用session->Send通过tcp连接发送通知给消息接收方的客户端




# 使用方法
```bash
# 1. 编译
cd ChatServer1
sh ./build

# 2. 启动
# 直接启动current_path不对读不到config.ini
cd build
./ChatServer1
```

# Debug
## 消息投递到队列里面后, 逻辑系统的后台线程没有进行处理
原因: 条件变量等待条件写错了
```c++
while (_msg_que.empty() && !_b_stop) {
    _cond.wait(unique_lk);
}
```
## 客户端无法连接到聊天服务
1. 客户端在宿主机, 服务端在虚拟机, `Host = 0.0.0.0`是访问不到的
2. 指定虚拟机`Host = 192.168.222.136`(根据实际服务所在ip进行更改)后, 报错
```bash
email is  "xliang9809@163.com"  uid is  1062  host is  "192.168.222.136" Port is  "8090"  Token is  "985469fc-0c9e-4c2a-ae0f-50e3d2dc9a48"
receive tcp connect signal
Connecting to server...
Error:  "The proxy type is invalid for this operation"
Other Error!
```
3. 关闭主机代理可解决以上问题

## 客户端无法接收界面切换信号
1. `CSession::Send`逻辑没有实现
    1. 通过打印日志+tcpdump(linux)+wireshark(windows)排查

## 好友申请确认后无法添加到联系人
1. `AuthFriendApply`逻辑没有实现
    1. 通过业务逻辑检查数据库状态位
2. `int friend_id = res->getInt("friend_id");` 获取错了数据类型

## 消息无法发送到对端
`std::string to_ip_key = USER_IP_PREFIX + std::to_string(touid);`查错了key

## 头像无法显示(客户端)
1. 头像路径是后面添加的, 先前缓存在redis中
2. 获取用户信息的逻辑是先查redis后查mysql
3. mysql中的数据更新了, 但是redis的数据还是旧的(没有头像信息), 因此不显示头像
4. 要处理数据库与缓存的一致性问题

## 程序关闭的时候显示core dump
1. gdb调试过程中加上`handle SIGSEGV stop print pass` -> 在core dump处停止
2. `signal SIGTERM` 终止程序
3. 使得core dump处可以停止 -> 查看bt
```bash
(gdb) bt
#0  0x0000555555626612 in boost::asio::io_context::work::get_io_context (this=0x0)
    at /usr/include/boost/asio/impl/io_context.hpp:428
#1  0x0000555555620ae5 in AsioIOContextPool::Stop (this=0x555557323ce0)
    at /home/tom/workspace/Feynman/ChatServer1/AsioIOContextPool.cpp:18
#2  0x00005555556209d2 in AsioIOContextPool::~AsioIOContextPool (
    this=0x555557323ce0, __in_chrg=<optimized out>
```
4. 发现主程序里面调用了Stop, 析构函数里面也调用了Stop -> 双重析构导致core dump
5. 在主程序里面控制, 接收到停止信号关闭iocontext池

## 跨服务聊天对方接收不到消息
1. 让两个客户端都登录到1个服务上, 看是否能够收到消息
    1. 如果能够收到, 说明服务传到客户端这个过程是通的
    2. server2的配置文件 `[chatserver1]`写错了



