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
#include "timer.h"

#define dataMaxSize 65536

bool becomeNonBlock(int fd) {
    //将lfd设为非阻塞模式
    int temp = fcntl(fd, F_GETFD);
    if (temp == -1) return false;
    if (fcntl(fd, F_SETFD, O_NONBLOCK | temp) == -1)
        return false;
    return true;
}

//可调用对象，即分发给线程池中线程的任务
class execTask {
    //产生epoll通知的文件描述符
    int fd;
    //主线程中创建的epoll实例
    int efd;
    //是否是lfd产生的I/O通知
    bool isListenFd;
    //时间堆
    timerHeap &t;
    //分配一条连接
//    mysqlConn *con = nullptr;
public:
//    explicit execTask(mysqlConn *mcon, int fd_, int efd_, bool isListenFd_, timerHeap &th) : con(mcon), fd(fd_),
//                                                                                             efd(efd_),
//                                                                                             isListenFd(isListenFd_),
//                                                                                             t(th) {
//    }

    explicit execTask(int fd_, int efd_, bool isListenFd_, timerHeap &th) : fd(fd_),
                                                                            efd(efd_),
                                                                            isListenFd(isListenFd_),
                                                                            t(th) {
    }

    //线程要执行的任务函数
    void operator()() {
        if (isListenFd) {
            int cfd;
            sockaddr_in clientaddr;
            socklen_t len;
            //尽可能多的读取I/O数据
            //acceptor用ev作为参数将套接字加入到epoll
            epoll_event ev;
            ev.events = EPOLLIN | EPOLLONESHOT | EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLET;
            while ((cfd = accept(fd, (sockaddr *) &clientaddr, &len)) > 0) {
                if (!becomeNonBlock(cfd)) {
                    close(cfd);
                    continue;
                }
                ev.data.fd = cfd;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev) == -1)
                    perror("thread add connect fd to epoll failed");
                    //添加定时器
                else t.pushTimer(myTimer(TIMEOUT, callWhich::disconnect, cfd, clientaddr));
            }
            if (errno != EWOULDBLOCK) {
                errExit("accept");
            }
            ev.data.fd = fd;
            ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) == -1)
                errExit("epoll 重置监听套接字失败");
        } else {
            char buf[dataMaxSize];
            memset(buf, 0, dataMaxSize);
            size_t n;
            if ((n = recv(fd, buf, dataMaxSize, 0)) < 0) {
                perror("recv");
                //删除定时器类，同时断开连接，epoll 清除
                t.eraseFd(fd);
            }
            if (!http::readAndSend(buf, fd)) {
                t.eraseFd(fd);
                return;
            }
            //延长定时器时间
            t.delayTimer(fd, TIMEOUT);
            //重置fd的EPOLLONESHOT
            epoll_event event;
            event.data.fd = fd;
            event.events = EPOLLIN | EPOLLONESHOT | EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLET;
            if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event) == -1) {
                if (errno != EEXIST) errExit("epoll 重置通信套接字失败");
            }
        }
    }
};


#endif //WEBSERVER_EXECTASK_H
