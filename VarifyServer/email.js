const nodemailer = require('nodemailer')
const config_module = require('./config')

// 发送邮件的代理
let transport = nodemailer.createTransport(
    {
        host: 'smtp.163.com',
        port: 465,
        secure: true,
        auth: {
            user: config_module.email_user,
            pass: config_module.email_pass
        }
    }
)

/**
 * 发送邮件的函数
 * @param {*} mailOptions_ 发送邮件的参数
 * @returns 
 */
function SendMail(mailOptions_) {
    return new Promise(function(resolve, reject) {
        transport.sendMail(mailOptions_, function(error, info) {
            if (error) {
                console.log(error)
                reject(error)
            } else {
                console.log('email has send: ' + info.response)
                resolve(info.response)
            }
        })
    })
}

module.exports.SendMail = SendMail