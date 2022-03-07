//
// Created by 17336 on 2022/3/6.
//

#ifndef WEBSERVER_EXECTASK_H
#define WEBSERVER_EXECTASK_H

#include <sys/socket.h>
#include "deal_error.h"
#include "sys/epoll.h"
#include <fstream>
#include "http.h"

#define dataMaxSize 65536

bool readAndSend(char *s,int fd);

//可调用对象，即分发给线程池中线程的任务
class execTask {
    //产生epoll通知的文件描述符
    int fd;
    //主线程中创建的epoll实例
    int efd;
    //是否是lfd产生的I/O通知
    bool isListenFd;
public:
    explicit execTask(int fd_) : fd(fd_), isListenFd(false) {}

    explicit execTask(int fd_,bool isListenFd_):fd(fd_),isListenFd(isListenFd_){}

    explicit execTask(int fd_, int efd_, bool isListenFd_) : fd(fd_), efd(efd_), isListenFd(isListenFd_) {
    }

    //线程要执行的任务函数
    void operator()() {
        if (isListenFd) {
            int cfd;
            sockaddr clientaddr;
            socklen_t len;
            //尽可能多的读取I/O数据
            //acceptor用ev作为参数将套接字加入到epoll
            epoll_event ev;
            ev.events = EPOLLIN | EPOLLONESHOT | EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLET;
            while ((cfd = accept(fd, &clientaddr, &len)) >0) {
                ev.data.fd = cfd;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev) == -1)
                    perror("thread add connect fd to epoll failed");
            }
            if (errno != EWOULDBLOCK) {
                errExit("accept");
            }
            epoll_event event;
            event.data.fd = fd;
            event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event) == -1)
                errExit("epoll 重置监听套接字失败");
        } else {
            char buf[dataMaxSize];
            memset(buf, 0, dataMaxSize);
            size_t n;
            if ((n = recv(fd, buf, dataMaxSize, 0)) < 0) {
                perror("recv");
                epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
            }
            std::cout<<buf;
            if(!http::readAndSend(buf,fd)){
                epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
            }
            //重置fd的EPOLLONESHOT
            epoll_event event;
            event.data.fd = fd;
            event.events = EPOLLIN | EPOLLONESHOT | EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLET;
            if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event) == -1){
                if(errno!=EEXIST) errExit("epoll 重置通信套接字失败");
            }
        }
    }
};


#endif //WEBSERVER_EXECTASK_H
