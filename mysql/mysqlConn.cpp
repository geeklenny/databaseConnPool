#include "mysqlConn.h"

MysqlConn::MysqlConn()
{

}

MysqlConn::MysqlConn(std::string url, std::string username, std::string password)
{
    m_url = url;
    m_username = username;
    m_password = password;
    m_stmtOfTestConn = MYSQLCONN_STMTOFTESTCONN;
    m_ulTimeOut = MYSQLCONN_TIMEOUT;

    try
    {
        m_driver = get_driver_instance();
    } catch (sql::SQLException &e)
    {
        std::cout << "Sql driver error";
    }
}

MysqlConn::~MysqlConn()
{
    this->m_driver = nullptr;
}

sql::Connection* MysqlConn::createDBConn()
{
    sql::Connection* conn;
    try
    {
        conn = m_driver->connect(m_url, m_username, m_password);
        return conn;
    } catch( sql::SQLException &e)
    {
        std::cout << "sql connect error";
        return nullptr;
    }
}

void MysqlConn::releaseDBConn( sql::Connection * conn)
{
    delete conn;
}

bool MysqlConn::bConnectionAvilable( sql::Connection * conn)
{
    try
    {
        sql::Statement* stmt = conn->createStatement();
        stmt->executeQuery(m_stmtOfTestConn);
        delete stmt;
        return true;
    } catch (sql::SQLException &e)
    {
        std::cout << "connect unavailible";
        return false;
    }
}

unsigned int MysqlConn::ulTimeForCheckTimeout()
{
    return m_ulTimeOut;
}
