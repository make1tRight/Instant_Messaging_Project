#include "MysqlDao.h"
#include "ConfigMgr.h"
#include "const.h"
#include "data.h"
#include <cppconn/prepared_statement.h>

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
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        if (conn == nullptr) {
            return -1;
        }
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
            return result;
        }
        return -1;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return -1;
    }
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
    auto conn = _pool->GetConnection();
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        if (conn == nullptr) {
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement("SELECT email FROM user WHERE name = ?"));
        stmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        
        while (res->next()) {
            std::cout << "Check email: " << res->getString("email") << std::endl;
            if (email != res->getString("email")) {
                return false;
            }
            return true;
        }
        return false;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& pwd) {
    auto conn = _pool->GetConnection();
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        if (conn == nullptr) {
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
        pstmt->setString(2, name);
        pstmt->setString(1, pwd);
        
        int update_count = pstmt->executeUpdate();
        std::cout << "Update rows: " << update_count << std::endl;
        std::cout << "new password: " << pwd << std::endl;
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userinfo) {
    auto conn = _pool->GetConnection();
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        if (conn == nullptr) {
            return false;
        }
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
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid) {
    auto conn = _pool->GetConnection();
    if (conn == nullptr) {
        return nullptr;
    }
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> userinfo = nullptr;
        while (res->next()) {
            userinfo.reset(new UserInfo);
            userinfo->_uid = uid;
            userinfo->_name = res->getString("name");
            userinfo->_email = res->getString("email");
            userinfo->_passwd = res->getString("pwd");
            userinfo->_nick = res->getString("nick");
            userinfo->_desc = res->getString("desc");
            userinfo->_sex = res->getInt("sex");
            userinfo->_icon = res->getString("icon");
            break;
        }
        return userinfo;
    }
    catch (sql::SQLException& e) {
        std::cout << "std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid) called" << std::endl;
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name) {
    auto conn = _pool->GetConnection();
    if (conn == nullptr) {
        return nullptr;
    }
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> userinfo = nullptr;
        while (!res->next()) {
            userinfo.reset(new UserInfo);
            userinfo->_uid = res->getInt("uid");
            userinfo->_name = name;
            userinfo->_email = res->getString("email");
            userinfo->_passwd = res->getString("pwd");
            userinfo->_nick = res->getString("nick");
            userinfo->_desc = res->getString("desc");
            userinfo->_sex = res->getInt("sex");
            userinfo->_icon = res->getString("icon");
            break;
        }
        return userinfo;
    }
    catch (sql::SQLException& e) {
        std::cout << "std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name) called" << std::endl;
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return nullptr;
    }
}

bool MysqlDao::AddFriendApply(const int& from, const int& to) {
    auto conn = _pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement(
                "INSERT INTO friend_apply (from_uid, to_uid) values (?, ?) "
                "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"
            )); //如果重复了用原来的值
        pstmt->setInt(1, from);
        pstmt->setInt(2, to);
        // 执行更新语句
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            return false;
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

bool MysqlDao::AddFriend(const int& from, const int& to, std::string backname) {
    auto conn = _pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        // 开始事务
        conn->setAutoCommit(false);
        std::unique_ptr<sql::PreparedStatement> pstmt1(conn->prepareStatement(
            "INSERT IGNORE INTO friend(self_id, friend_id, back) VALUES (?, ?, ?)"));
        pstmt1->setInt(1, from);
        pstmt1->setInt(2, to);
        pstmt1->setString(3, backname);
        int rowAffected = pstmt1->executeUpdate();
        if (rowAffected < 0) {
            conn->rollback();
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt2(conn->prepareStatement(
            "INSERT IGNORE INTO friend(self_id, friend_id, back) VALUES (?, ?, ?)"));
        pstmt2->setInt(1, to);
        pstmt2->setInt(2, from);
        pstmt2->setString(3, "");
        rowAffected = pstmt2->executeUpdate();
        if (rowAffected < 0) {
            conn->rollback();
            return false;
        }
        // 提交事务
        conn->commit();
        std::cout << "AddFriend called success." << std::endl;
        return true;
    }
    catch (sql::SQLException& e) {
        if (conn) {
            conn->rollback();
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

bool MysqlDao::GetApplyList(int touid, 
    std::vector<std::shared_ptr<ApplyInfo>>& apply_list, int begin, int limit) {
    auto conn = _pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT apply.from_uid, apply.status, user.name, "
            "user.nick, user.sex FROM friend_apply AS apply JOIN user ON apply.from_uid = user.uid WHERE apply.to_uid = ? "
            "AND apply.id > ? ORDER BY apply.id ASC LIMIT ? "));
        pstmt->setInt(1, touid);    //将uid替换为要查询的uid
        pstmt->setInt(2, begin);    //起始id
        pstmt->setInt(3, limit);    //偏移差
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 遍历结果集
        while (res->next()) {
            auto name = res->getString("name");
            auto uid = res->getInt("from_uid");
            auto status = res->getInt("status");
            auto nick = res->getString("nick");
            auto sex = res->getInt("sex");
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
            apply_list.push_back(apply_ptr);
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

bool MysqlDao::GetFriendList(int self_id,
    std::vector<std::shared_ptr<UserInfo>>& user_list) {
    auto conn = _pool->GetConnection();
    if (conn == nullptr) {
        return false;
    }
    Defer defer([this, &conn]() {
        _pool->ReturnConnection(std::move(conn));
    });
    try {
        // 找到自己关联的所有好友
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT * FROM friend WHERE self_id = ?"));
        pstmt->setInt(1, self_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next()) {
            std::string friend_id = res->getString("friend_id");
            std::string back = res->getString("back");
            std::shared_ptr<UserInfo> userinfo = GetUser(friend_id);
            if (userinfo == nullptr) {
                continue;
            }
            userinfo->_back = userinfo->_name;
            user_list.push_back(userinfo);
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )." << std::endl;
        return false;
    }
}

