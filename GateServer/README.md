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

## 配置管理层
### ConfigMgr
1. 将配置信息统一写在config.ini中
2. 配置管理类统一读取并设置对应参数
3. 配置有全局不变性, 实现为单例模式

## 数据访问层

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
