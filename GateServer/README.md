# GateServer
用于建立http短连接, 主要面向注册, 修改密码, 登录等功能
### GateServer
1. 服务器启动与优雅退出

## 网络层
### CServer
1. 负责监听新的客户连接+分发连接

### HttpConnection
1. 负责读写, 将结果提交至逻辑队列
2. 根据http请求Get/Post, 分发到逻辑系统中进行处理

## 逻辑层
### LogicSystem
1. 负责将逻辑队列中的消息取出, 根据消息id, 选择对应回调函数进行处理
2. 采用注册-处理机制来管理http连接请求, 不管请求类型怎么增加, 都不需要改变代码结构
3. unordered_map实现了O(1)查找时间复杂度
4. 主要逻辑说明:
    1. `/get_test` Get请求处理(get报文测试, 可删除)
    2. `/get_varifycode` Post请求处理(获取验证码请求)


# 使用方法
```bash
# 1. 编译
cd GateServer
sh ./build

# 2. 启动
./build/GateServer
```

# Debug
- `bad_weak_ptr` 错误 -> 继承忘记加public
1. 使用`shared_from_this()`方法的类
    - 必须使用`std::shared_ptr<T>`进行管理
    - 必须继承`public std::enable_shared_from_this<T>`
2. 以上两条如果缺少, 会出现`bad_weak_ptr`的exception