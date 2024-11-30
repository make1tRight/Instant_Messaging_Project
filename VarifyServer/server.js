const grpc = require('@grpc/grpc-js')
const message_proto = require('./proto')
const const_module = require('./const')
const { v4: uuidv4 } = require('uuid')
const emailModule = require('./email')
const redis_module = require('./redis')

async function GetVarifyCode(call, callback) {
    console.log("email is ", call.request.email)
    try{
        //code_xliang9809@163.com
        let query_res = await redis_module.GetRedis(const_module.code_prefix + call.request.email)  //前缀加邮箱地址就是key
        console.log("query_res is ", query_res)
        let uniqueId = query_res
        if (query_res == null) {
            uniqueId = uuidv4()
            if (uniqueId.length > 4) {
                uniqueId = uniqueId.substring(0, 4) //取前4位
            }
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix + call.request.email, uniqueId, 600)
            if (!bres) {
                callback(null, {
                    email: call.request.email,
                    error: const_module.Errors.RedisErr
                })
                return
            }
        }
        console.log("uniqueId is ", uniqueId)
        let text_str =  '您的验证码为'+ uniqueId +'请十分钟内完成注册'
        //发送邮件
        let mailOptions = {
            from: 'xliang9809@163.com', //从这个邮箱发出
            to: call.request.email,
            subject: '验证码',
            text: text_str,
        };
    
        let send_res = await emailModule.SendMail(mailOptions); //await等待promise完成, await一定要在async中使用
        console.log("send res is ", send_res)

        callback(null, { 
            email:  call.request.email,
            error:const_module.Errors.Success
        })

        if (!send_res) {
            callback(null, {
                email: call.request.email,
                error: const_module.Errors.RedisErr
            })
            return
        }
        
 
    }catch(error){ //邮箱发送失败
        console.log("catch error is ", error)

        callback(null, { email:  call.request.email,
            error:const_module.Errors.Exception
        }); 
    }
     
}

function main() {
    var server = new grpc.Server()
    server.addService(message_proto.VarifyService.service, { GetVarifyCode: GetVarifyCode })
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        server.start()
        console.log('grpc server started')        
    })
}

main()