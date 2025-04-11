#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <cppconn/prepared_statement.h>
#include "const.h"

MysqlConnPool::MysqlConnPool(const std::string& url, const std::string& usr, const std::string& pwd,
    const std::string& schema, std::size_t poolSize)
    : _url(url), _usr(usr), _pwd(pwd), _schema(schema),
     _poolSize(poolSize), _b_stop(false) {
    try {
        for (int i = 0; i < _poolSize; ++i) {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> conn(driver->connect(_url, _usr, _pwd));
            conn->setSchema(_schema);
            _pool.push(std::move(conn));
        }
    }
    catch (sql::SQLException& e) {
        std::cout << "Failed to init mysql pool" << std::endl;
    }
}

std::unique_ptr<sql::Connection> MysqlConnPool::GetConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this]() {
        if (_b_stop) {
            return true;
        }
        return !_pool.empty();
    });
    if (_b_stop) {
        return nullptr;
    }
    std::unique_ptr<sql::Connection> conn(std::move(_pool.front()));
    _pool.pop();
    return conn;
}

void MysqlConnPool::ReturnConnection(std::unique_ptr<sql::Connection> conn) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop) {
        return;
    }
    _pool.push(std::move(conn));
    lock.unlock();
    _cond.notify_one();
}

void MysqlConnPool::Close() {
    _b_stop = true;
    _cond.notify_all();
}


MysqlConnPool::~MysqlConnPool() {
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_pool.empty()) {
        _pool.pop();
    }
}

MysqlDao::MysqlDao() {
    auto cfg = ConfigMgr::GetInstance();
    const std::string& host = (*cfg)["Mysql"]["Host"];
    const std::string& port = (*cfg)["Mysql"]["Port"];
    const std::string& user = (*cfg)["Mysql"]["User"];
    const std::string& passwd = (*cfg)["Mysql"]["Passwd"];
    const std::string& schema = (*cfg)["Mysql"]["Schema"];

    _pool = std::make_unique<MysqlConnPool>(host + ":" + port,
            user, passwd, schema, 5);
}

MysqlDao::~MysqlDao() {
    _pool->Close();
}

int MysqlDao::UserRegister(const std::string& name,
    const std::string& email, const std::string& passwd) {
    auto conn = _pool->GetConnection();
    try {
        if (conn == nullptr) {
            return -1;
        }
        Defer defer([this, &conn]() {
            _pool->ReturnConnection(std::move(conn));
        });
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("CALL reg_user(?,?,?,@result)"));
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, passwd);
        stmt->execute();
        std::unique_ptr<sql::Statement> stmtResult(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmtResult->executeQuery("SELECT @result AS result"));
        
        if (res->next()) {
            int result = res->getInt("result");
            std::cout << "Result: " << result << std::endl;
            // _pool->ReturnConnection(std::move(conn));
            return result;
        }
        // _pool->ReturnConnection(std::move(conn));
        return -1;
    }
    catch (sql::SQLException& e) {
        // _pool->ReturnConnection(std::move(conn));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return -1;
    }
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
    auto conn = _pool->GetConnection();
    try {
        if (conn == nullptr) {
            return false;
        }
        Defer defer([this, &conn]() {
            _pool->ReturnConnection(std::move(conn));
        });
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("SELECT email FROM user WHERE name = ?"));
        stmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        
        while (res->next()) {
            std::cout << "Check email: " << res->getString("email") << std::endl;
            if (email != res->getString("email")) {
                // _pool->ReturnConnection(std::move(conn));
                return false;
            }
            // _pool->ReturnConnection(std::move(conn));
            return true;
        }
        return false;
    }
    catch (sql::SQLException& e) {
        // _pool->ReturnConnection(std::move(conn));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& pwd) {
    auto conn = _pool->GetConnection();
    try {
        if (conn == nullptr) {
            return false;
        }
        Defer defer([this, &conn]() {
            _pool->ReturnConnection(std::move(conn));
        });
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
        pstmt->setString(2, name);
        pstmt->setString(1, pwd);
        
        int update_count = pstmt->executeUpdate();
        std::cout << "Update rows: " << update_count << std::endl;
        std::cout << "new password: " << pwd << std::endl;
        // _pool->ReturnConnection(std::move(conn));
        return true;
    }
    catch (sql::SQLException& e) {
        // _pool->ReturnConnection(std::move(conn));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userinfo) {
    auto conn = _pool->GetConnection();
    try {
        if (conn == nullptr) {
            return false;
        }
        Defer defer([this, &conn]() {
            _pool->ReturnConnection(std::move(conn));
        });
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT * FROM user WHERE email = ?"));
        pstmt->setString(1, email);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";
        while (res->next()) {
            origin_pwd = res->getString("pwd");
            std::cout << "Password: " << origin_pwd << std::endl;
            break;
        }
        if (pwd != origin_pwd) {
            return false;
        }
        userinfo._uid = res->getInt("uid");
        userinfo._email = res->getString("email");
        userinfo._name = res->getString("name");
        userinfo._passwd = origin_pwd;
        return true;
    }
    catch (sql::SQLException& e) {
        // _pool->ReturnConnection(std::move(conn));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

