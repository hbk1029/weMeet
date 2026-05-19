#include "../MySQL/MySQL.h"
#include <iostream>
using namespace std;

bool MySQL::ConnectMysql(const char* server,const char* user,const char* password, const char* database)
{
    conn = mysql_init(NULL);
    if (!conn) {
        cout << "mysql_init failed" << endl;
        return false;
    }
    bool reconnect = true;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
    if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0))
    {
        cout << "mysql_real_connect failed: " << mysql_error(conn) << endl;
        return false;
    }
    mysql_set_character_set(conn,"utf8");
    pthread_mutex_init(&m_lock, NULL);
    return true;
}

bool MySQL::SelectMysql(char* szSql, int nColumn, list<string>& lst)
{
    MYSQL_RES* results = NULL;
    pthread_mutex_lock(&m_lock);
    if(mysql_query(conn,szSql)) {
        cout << "SelectMysql error: " << mysql_error(conn) << " sql: " << szSql << endl;
        pthread_mutex_unlock(&m_lock);
        return false;
    }
    results = mysql_store_result(conn);
    pthread_mutex_unlock(&m_lock);
    if(NULL == results)return false;
    MYSQL_ROW record;
    while((record = mysql_fetch_row(results)))
    {
        for(int i=0; i<nColumn; i++)
        {
            lst.push_back( record[i] );
        }
    }
    return true;
}

bool MySQL::UpdateMysql(char *szsql)
{
    if(!szsql)return false;
    pthread_mutex_lock(&m_lock);
    if(mysql_query(conn,szsql)){
        cout << "UpdateMysql error: " << mysql_error(conn) << " sql: " << szsql << endl;
        pthread_mutex_unlock(&m_lock);
        return false;
    }
    pthread_mutex_unlock(&m_lock);
    return true;
}

void MySQL::DisConnect()
{
    mysql_close(conn);
}
