const config_module = require('./config')
const Redis = require('ioredis')


// 创建Redis客户端实例
const RedisCli = new Redis({
    host: config_module.redis_host,         //Redis服务器主机名
    port: config_module.redis_port,         //Redis服务器端口号
    password: config_module.redis_passwd,   //Redis服务器密码
});

// 监听错误信息
RedisCli.on("error", function (err) {
    console.log("RedisCli connect error")
    RedisCli.quit()
});

/**
 * 根据key获取value
 * @param {*} key
 * @returns
 */
async function GetRedis(key) { //用await要做async声明
    try {
        const result = await RedisCli.get(key)
        if (result == null) {
            console.log('result:', '<' + result + '>', 'This key cannot be found...')
            return null;
        }
        console.log('Result:', '<' + result + '>', 'Get key successfully!...')
        return result
    } catch(error) {
        console.log('GetRedis error is', error)
        return null
    }
}

/**
 * 根据key查询redis是否存在keys
 * @param {*} key
 * @returns
 */
async function QueryRedis(key) {
    try {
        const result = await RedisCli.exists(key)
        if (result == 0) {
            console.log('result:', '<' + result + '>', 'This key is null...')
            return null;
        }
        console.log('Result:', '<' + result + '>', 'With this value!...')
        return result
    } catch(error) {
        console.log('QueryRedis error is', error)
        return null
    }
}

/**
 * 设置key, value和过期时间
 * @param {*} key
 * @param {*} value
 * @param {*} exptime
 * @returns
 */
async function SetRedisExpire(key, value, exptime) {
    try {
        //设置key-value
        await RedisCli.set(key, value)
        //设置过期时间
        await RedisCli.expire(key, exptime)
        return true
    } catch(error) {
        console.log('SetRedisExpire error is', error)
        return false
    }
}

/**
 * 退出函数
 */
function Quit() {
    RedisCli.quit()
}

module.exports = {GetRedis, QueryRedis, Quit, SetRedisExpire} //要把接口抛出去