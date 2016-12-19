#ifndef _COMMON_DB_CONN_POOL_
#define _COMMON_DB_CONN_POOL_

#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std;


enum DBCONNSTATS
{
    DB_CONN_FREE,
    DB_CONN_USED,
    DB_CONN_TEMP_USED

};

template<typename T> class CommonDbConnPool;

template <typename T> ostream& operator<<(ostream &, CommonDbConnPool<T> &);

template<typename T>
class CommonDbConnPool
{
  public:

    virtual int InitPool(int minConn, int maxConn);
    void PoolConnMonitor();
    bool IsPoolVaild();
    virtual T* GetDBConnFromPool();
    virtual int DestroyPool();
    virtual void ReleaseDbConnToPool(T*);

    friend ostream& operator<< <> (ostream &, CommonDbConnPool<T> &);


  protected:
    virtual T* createDBConn() = 0;
    virtual void releaseDBConn(T*) = 0;
    virtual bool bConnectionAvilable(T*) = 0;
    virtual unsigned int ulTimeForCheckTimeout() = 0;

  private:

    int m_minConn;
    int m_maxConn;
    map<T*, DBCONNSTATS> m_poolMap;
    bool m_bPoolAvailable;
    bool m_bPoolToDestroy;    // pool goes to destory

    mutex m_mtxPool;

    mutex m_mtxCv;
    condition_variable m_Cv;
    thread m_trdConnMonitor;

};


template<typename T>
int CommonDbConnPool<T>::InitPool( int minConn, int maxConn )
{
    m_minConn = minConn;
    m_maxConn = maxConn;
    m_bPoolAvailable = false;

    T* t;
    for ( int i =0 ; i < minConn; i++ )
    {
        if( (t = createDBConn()) != nullptr )
        {
            if( (bConnectionAvilable(t)) )
            {
                m_poolMap[t] = DB_CONN_FREE;
            }
            else
            {
                releaseDBConn(t);
            }
        }
    }

    if(m_poolMap.size() == 0)
        return -1;

    m_bPoolToDestroy = false;

    thread th(&CommonDbConnPool<T>::PoolConnMonitor, this);

    swap(th, m_trdConnMonitor);

    m_bPoolAvailable = true;
    return m_poolMap.size();
}



template <typename T>
void CommonDbConnPool<T>::PoolConnMonitor(){
    CommonDbConnPool<T> * ptrPool = this;

    while(ptrPool->m_bPoolAvailable && ! ptrPool->m_bPoolToDestroy)
    {
        [&](){
            lock_guard<mutex> locker(ptrPool->m_mtxPool);
            for(auto &iter : ptrPool->m_poolMap)
            {
                if(iter.second == DB_CONN_FREE)
                {
                    if( !ptrPool->bConnectionAvilable(iter.first) )
                        ptrPool->releaseDBConn(iter.first);
                }
            }
        }();

        [&](){
            unique_lock<mutex> lk(ptrPool->m_mtxCv);
            if(ptrPool->m_Cv.wait_for(lk, chrono::milliseconds(ptrPool->ulTimeForCheckTimeout())) == cv_status::timeout)
            {
                ptrPool->m_bPoolAvailable = false;
            }
        }();

    }
}

template <typename T>
T* CommonDbConnPool<T>::GetDBConnFromPool()
{
    lock_guard<mutex> locker(m_mtxPool);
    T* t = nullptr;

    for(auto &iter : m_poolMap)
    {
        if(iter.second == DB_CONN_FREE){
            iter.second = DB_CONN_USED;
            t = iter.first;
            break;
        }
    }

    if (t == nullptr){
        if(m_poolMap.size() < m_maxConn)
        {
            if( (t = createDBConn()) != nullptr )
            {
                if( (bConnectionAvilable(t)) )
                {
                    m_poolMap[t] = DB_CONN_TEMP_USED;
                }
                else
                {
                    releaseDBConn(t);
                }
            }
        }


    }

    return t;
}

template <typename T>
void CommonDbConnPool<T>::ReleaseDbConnToPool(T* t)
{
    lock_guard<mutex> locker(m_mtxPool);
    for (auto &iter : m_poolMap)
    {
        if(iter.first == t)
        {
            if(iter.second == DB_CONN_USED)
            {
                iter.second = DB_CONN_FREE;
            } else if (iter.second == DB_CONN_TEMP_USED)
            {
                releaseDBConn(iter.first);
                m_poolMap.erase(iter.first);
            }
        }
    }
}

template <typename T>
int CommonDbConnPool<T>::DestroyPool(){

    cout << m_poolMap.size() << endl;
    for( auto &iter : m_poolMap)
    {
        if( iter.second == DB_CONN_FREE)
        {
            releaseDBConn(iter.first);
            m_poolMap.erase(iter.first);
        }
    }

    m_bPoolToDestroy = true;

    m_Cv.notify_one();

    m_trdConnMonitor.join();

    cout << m_poolMap.size() << endl;
    return m_poolMap.size();
}

template <typename T>
bool CommonDbConnPool<T>::IsPoolVaild()
{
    return m_bPoolAvailable;
}

template <typename T>
ostream& operator<<(ostream& os,  CommonDbConnPool<T> & CdbPool){

    os << "---Pool is " << (CdbPool.IsPoolVaild()?"valid":"invalid") << "----" << endl;
    os << "Keepalive every " << CdbPool.ulTimeForCheckTimeout()/1000 << " seconds\n";

    for( const auto &iter : CdbPool.m_poolMap )
    {
        os << "pointer : " << iter.first << " : " ;
        switch(iter.second)
        {
            case DB_CONN_FREE:
                os << "free" << endl;
                break;
            case DB_CONN_USED:
                os << "used" << endl;
                break;
            case DB_CONN_TEMP_USED:
                os << "temp" << endl;
                break;
        }
    }
    os << endl;
    return os;
}

#endif

