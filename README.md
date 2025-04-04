# Feynman
## GateServer
网关服务 - 用于建立http短连接, 主要面向注册, 修改密码, 登录等功能

## VarifyServer
验证服务 - 发送验证码以增加安全性, 用于账号的注册, 修改密码等逻辑

## StatusServer
状态服务 - 实现负载均衡, 查询负载更低的聊天服务信息(ip+port+token)用于客户端建立tcp长连接

## ChatServer
聊天服务 - 用于建立tcp长连接, 主要面向好友申请, 好友认证, 消息转发等功能