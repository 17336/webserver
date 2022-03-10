//
// Created by 17336 on 2022/2/24.
//

#include <netdb.h>
#include <thread>
#include <iostream>
#include "inet_sockets.h"
#include "deal_error.h"
#include <sys/wait.h>
#include "threadsafe_queue.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include "execTask.h"


#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

#include "threadPool.h"

#define MAX_EVENTS 4096

int main(int argc, char **argv) {
    socklen_t len;
    int lfd;
    //创建监听连接套接字lfd
    if ((lfd = inetListen("60200", 1000, &len)) < 0) {
        errExit("inetListen");
    }
    //将lfd设为非阻塞模式
    int temp = fcntl(lfd, F_GETFD);
    if (temp == -1)
        errExit("fcntl");
    if (fcntl(lfd, F_SETFD, O_NONBLOCK | temp) == -1)
        errExit("fcntl");

    //创建epoll实例
    int efd;
    if ((efd = epoll_create1(0)) == -1) {
        errExit("epoll create");
    }

    //evlist用于epoll_wait的返回值(为了节省空间，evlist[0]同时用作epoll_ctl函数的参数)
    epoll_event evlist[MAX_EVENTS];
    //将监听套接字加入epoll实例，同时设置为边缘触发模式+一次触发
    evlist[0].events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    evlist[0].data.fd = lfd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, lfd, evlist) == -1) {
        errExit("add listen fd failed");
    }

    threadPool tPool;
    while (true) {
        int ready;
        //获取已经准备I/O的文件描述符
        ready = epoll_wait(efd, evlist, MAX_EVENTS, -1);
        if (ready == -1) {
            if (errno == EINTR) continue;
            errExit("epoll_wait");
        }
        for (int i = 0; i < ready; ++i) {
            //如果是lfd准备好accept
            if (evlist[i].data.fd == lfd) {
                execTask task(lfd,efd, true);
                tPool.pushTask(task);
            } else {
                //如果对端挂断或者出错，将其关闭并删除
                if (evlist[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                    epoll_ctl(efd, EPOLL_CTL_DEL, evlist[i].data.fd, nullptr);
                    close(evlist[i].data.fd);
                } else {
                    execTask task(evlist[i].data.fd);
                    tPool.pushTask(task);
                }
            }
        }
    }
}


#pragma clang diagnostic pop