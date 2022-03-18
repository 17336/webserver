//
// Created by 17336 on 2022/2/24.
//

#ifndef WEBSERVER_INET_SOCKETS_H
#define WEBSERVER_INET_SOCKETS_H


#include <sys/socket.h>
#include <string>
#include <cstring>
#include <netdb.h>
#include <iostream>
#include <cstdio>
#include <unistd.h>

int inetConnect(const char *host, const char *service, int type);
int inetListen(const char *service, int backlog, socklen_t *addrlen);
int inetBind(const char *service, socklen_t *addrlen, int type);



#endif //WEBSERVER_INET_SOCKETS_H
