#include "MysqlDao.h"
#include "ConfigMgr.h"

MysqlDao::MysqlDao() {
    auto& cfg = ConfigMgr::Inst();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port= cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MysqlPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
    pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd) {
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?, ?, ?, @result)"));
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);
        stmt->execute();
        // PreparedStatement��֧��ע���������, ��Ҫʹ�ûỰ����������������ȡ���������ֵ
        // ����洢���������˻Ự����, ����������ʽ��ȡ���������ֵ, ����������ִ��SELECT��ѯ����ȡ����
        std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next()) {
            int result = res->getInt("result");
            std::cout << "Result: " << result << std::endl;
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return -1;
    }
    catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cout << "RegUser called" << std::endl;
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }
        //׼����ѯ���
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));
        //�󶨲���
        pstmt->setString(1, name);
        //ִ�в�ѯ
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        //���������
        while (res->next()) {
            std::cout << "Check Email: " << res->getString("email") << std::endl;
            if (email != res->getString("email")) {
                pool_->returnConnection(std::move(con));
                return false;
            }
            pool_->returnConnection(std::move(con));
            return true;
        }
        return true;
    }
    catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cout << "CheckEmail called" << std::endl;
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }
        //׼����ѯ���
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
        //�󶨲���
        pstmt->setString(2, name);      //�ڶ����ʺ�
        pstmt->setString(1, newpwd);    //��һ���ʺ�
        //ִ��UPDATE
        int updateCount = pstmt->executeUpdate();
        std::cout << "Updated rows: " << updateCount << std::endl;
        pool_->returnConnection(std::move(con));
        return true;
    }
    catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userinfo) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });
    try {
        //׼��SQL���
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
        pstmt->setString(1, email);
        //ִ�в�ѯ
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";
        //���������
        while (res->next()) {
            origin_pwd = res->getString("pwd");
            //�����ѯ��������
            std::cout << "Password: " << origin_pwd << std::endl;
            break;
        }
        if (pwd != origin_pwd) {
            return false;
        }
        userinfo.name = res->getString("name");
        userinfo.email = email;
        userinfo.uid = res->getInt("uid");
        userinfo.pwd = origin_pwd;
        return true;
    }
    catch (sql::SQLException& e) {
        std::cout << "CheckPwd called" << std::endl;
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return nullptr;
    }
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });
    try {
        //׼��SQL���
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid);
        //ִ�в�ѯ
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        //���������
        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name= res->getString("name");
            //user_ptr->nick = res->getString("nick");
            //user_ptr->desc = res->getString("desc");
            //user_ptr->sex = res->getInt("sex");
            //user_ptr->icon = res->getString("icon");
            user_ptr->uid = uid;
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e) {
        std::cout << "std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid) called" << std::endl;
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return nullptr;
    }
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });
    try {
        //׼��SQL���
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);
        //ִ�в�ѯ
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        //���������
        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->icon = res->getString("icon");
            user_ptr->sex = res->getInt("sex");
            user_ptr->uid = res->getInt("uid");
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e) {
        std::cout << "std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name) called" << std::endl;
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}
