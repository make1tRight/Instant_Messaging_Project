# VarifyServer
发送验证码以增加安全性, 用于账号的注册, 修改密码等逻辑

## proto.js
1. 用于gRPC的message.proto文件解析
2. 导出定义的消息message与服务接口, 用于后续的gRPC通信

## config.json
- 读取配置
    1. 邮箱授权码
    2. redis登录信息

## const.js
- 存储常量和全局变量

## config.js
- 用于读取配置
    1. 发邮件的邮箱
    2. 发邮件所需的授权密码
    3. 验证码前缀

## email.js
- 异步发送邮件, 失败则reject, 成功则resolve

## redis.js
1. 建立varifyserver和redis的连接
2. `SetRedisExpire`将发送出去的验证码缓存到redis中并设置过期时间

## server.js
1. 启动grpc server
2. 定义供外部调用的GetVarifyCode
    1. 生成uuid
    2. 调用email.js中的SendMail方法

# Debug
## 邮件无法发送, 返回RPC_FAILED错误
- 发邮件不能开代理, 否则端口会被重定向到7890去
- 不是50051无法和验证服务正常通讯