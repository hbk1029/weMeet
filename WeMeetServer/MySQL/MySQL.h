#ifndef _MYSQL_H
#define _MYSQL_H
#include "../Net/def.h"
#include <mysql/mysql.h>
#include<list>
#include<string>

using namespace std;


class MySQL {
public:
    bool ConnectMysql(const char *server, const char *user, const char *password, const char *database);
    bool SelectMysql(char* szSql,int nColumn,list<string>& lst);
    bool UpdateMysql(char *szsql);
    void DisConnect();

private:
    MYSQL *conn;
    pthread_mutex_t m_lock;
};




#endif
