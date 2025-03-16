const grpc = require('@grpc/grpc-js')
const message_proto = require('./proto')
const const_module = require('./const')
const { v4: uuidv4 } = require('uuid')
const email_module = require('./email')

// client调用的是下面这个方法
async function GetVarifyCode(call, callback) {
    console.log("email: ", call.request.email)
    try {
        uniqueId = uuidv4()
        console.log("uniqueId: ", uniqueId)
        let text_str = 
            'your varify code: ' + uniqueId + ', please register in 3 minutes.'
        let mailOptions = {
            from: 'xliang9809@163.com',
            to: call.request.email,
            subject: 'varify code',
            text: text_str
        }

        let send_res = await email_module.SendMail(mailOptions)
        console.log("send res: ", send_res)

        callback(null, {
            email: call.request.email,
            error: const_module.ERROR_CODE.SUCCESS
        })
    }
    catch (error) {
        console.log('catch error: ', error)
        callback(null, {
            email: call.request.email,
            error: const_module.ERROR_CODE.EXCEPTION
        })
    }
}

function main() {
    var server = new grpc.Server()
    server.addService(
        message_proto.VarifyService.service,
        {
            GetVarifyCode: GetVarifyCode
        } 
    )
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        console.log('grpc server started.')
    })
}

main()