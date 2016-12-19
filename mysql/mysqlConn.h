#ifndef _MYSQL_CONN_
#define _MYSQL_CONN_

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include "../CommonDbConnPool.h"
#include "mysqlConnConst.h"

class MysqlConn : public CommonDbConnPool<sql::Connection>
{
  public:

    MysqlConn(std::string url, std::string username, std::string password);
    virtual ~MysqlConn();

    sql::Connection* createDBConn();
    void releaseDBConn(sql::Connection*);
    bool bConnectionAvilable(sql::Connection*);
    unsigned int ulTimeForCheckTimeout();

  private:

    MysqlConn();

    sql::Driver* m_driver;
    std::string m_url;
    std::string m_username;
    std::string m_password;
    std::string m_stmtOfTestConn;    // statment of test mysql connection
    unsigned int m_ulTimeOut;    // default value of timeout

};

#endif
