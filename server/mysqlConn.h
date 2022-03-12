//
// Created by 17336 on 2022/3/11.
//

#ifndef WEBSERVER_MYSQLCONN_H
#define WEBSERVER_MYSQLCONN_H

#include <mysql/mysql.h>
#include <vector>
#include <mutex>

class mysqlConn {
    MYSQL data;
public:
    bool isConnected;

    mysqlConn(const char *host, const char *user, const char *passwd, const char *db, unsigned int port,
              const char *unix_socket, unsigned long client_flag) {
        if (mysql_init(&data) == nullptr ||
            mysql_real_connect(&data, host, user, passwd, db, port, unix_socket, client_flag) == nullptr) {
            isConnected = false;
            return;
        }
        isConnected = true;
    }

    ~mysqlConn() {
        mysql_close(&data);
    }
};

class mysqlConnPool {
    std::vector<mysqlConn *> pool;
    std::mutex mut;
public:
    mysqlConnPool() {}

    mysqlConnPool(int n, const char *host,
                  const char *user,
                  const char *passwd,
                  const char *db,
                  unsigned int port,
                  const char *unix_socket,
                  unsigned long client_flag) {
        for (int i = 0; i < n; ++i) {
            mysqlConn *temp=new mysqlConn(host,user,passwd,db,port,unix_socket,client_flag);
            pool.push_back(temp);
        }
    }

    mysqlConn *getConn(){
        std::lock_guard<std::mutex> lock(mut);
        if(pool.empty()) return nullptr;
        mysqlConn *p=pool.back();
        pool.pop_back();
        return p;
    }

    void push(mysqlConn *p){
        std::lock_guard<std::mutex> lock(mut);
        pool.push_back(p);
    }

};

#endif //WEBSERVER_MYSQLCONN_H
