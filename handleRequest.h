//
// Created by 17336 on 2022/2/27.
//

#ifndef WEBSERVER_HANDLEREQUEST_H
#define WEBSERVER_HANDLEREQUEST_H


#include <sys/socket.h>
#include <iostream>
#include <cstring>

class handleRequest {
    int fd;
public:
    handleRequest(int a) : fd(a) {

    }
    void run(){
        char buf[100] = "";
        recv(fd, buf, 13, 0);
        std::cout << buf << std::endl;
        strcpy(buf, "你好");
        send(fd, buf, 10, 0);
    }

};


#endif //WEBSERVER_HANDLEREQUEST_H
