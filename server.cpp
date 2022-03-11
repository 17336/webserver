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
#include "timer.h"
#include <sys/resource.h>
#include <unistd.h>


#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

#include "threadPool.h"

#define MAX_EVENTS 4096
#define MAX_OPENS 100000

int main(int argc, char **argv) {
    //修改当前的最大打开文件数
    struct rlimit r;
    r.rlim_max = MAX_OPENS;
    r.rlim_cur = MAX_OPENS;
    if (setrlimit(RLIMIT_NOFILE, &r) == -1) {
        perror("setrlimit");
    }

    socklen_t len;
    int lfd;
    //创建监听连接套接字lfd
    if ((lfd = inetListen("60200", 20000, &len)) < 0) {
        errExit("inetListen");
    }

    if(!becomeNonBlock(lfd)) errExit("no block");

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

    //epoll超时时间
    time_t timeout = TIMEOUT;
    timerHeap tHeap(efd);
    while (true) {
        int ready;
        //记录epoll_wait开始时间和超时结束时间
        time_t start = time(nullptr), end = start + timeout;
        //获取已经准备I/O的文件描述符
        ready = epoll_wait(efd, evlist, MAX_EVENTS, timeout);
        if (ready == -1) {
            //因为信号中断epoll而返回，则重置timeout
            if (errno == EINTR) {
                timeout = end - time(nullptr);
                if (timeout <= 0) timeout = tHeap.getLatestTime();
                continue;
            }
            errExit("epoll_wait");
        }
        //epoll因为超时而返回
        if (ready == 0) {
            //处理定时事件，并重置超时时间
            timeout = tHeap.tick();
        } else {
            for (int i = 0; i < ready; ++i) {
                //如果是lfd准备好accept，交给线程池处理数据和定时器
                if (evlist[i].data.fd == lfd) {
                    execTask task(lfd, efd, true, tHeap);
                    tPool.pushTask(task);
                }
                    //如果对端挂断或者出错，直接由主线程执行任务：删除其定时器
                else if (evlist[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                    tHeap.eraseFd(evlist[i].data.fd);
                }
                    //有数据，则交给线程池处理数据和定时器
                else {
                    execTask task(evlist[i].data.fd, efd, false, tHeap);
                    tPool.pushTask(task);
                }
            }
            timeout = end - time(nullptr);
            if (timeout <= 0) timeout = tHeap.getLatestTime();
        }
    }
}


#pragma clang diagnostic pop