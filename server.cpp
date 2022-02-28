//
// Created by 17336 on 2022/2/24.
//

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include "inet_sockets.h"
#include "deal_error.h"
#include <sys/wait.h>
#include "handleRequest.h"


#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main(int argc, char **argv) {
    socklen_t len;
    int lfd;
    if ((lfd = inetListen("60200", 1000, &len)) < 0) {
        errExit("inetListen");
    }
    while (1) {
        int cfd;
        sockaddr clientAddr;
        socklen_t clientLen;
        if ((cfd = accept(lfd, &clientAddr, &clientLen)) < 0) continue;
        pid_t pid;
        if ((pid = fork()) < 0) {
            perror("fork");
            continue;
        } else if (pid > 0) {//父进程
            close(cfd);
            //子进程立即终止后，父进程立即回收子进程资源
            waitpid(pid, nullptr, 0);
        } else {//子进程，再次fork让子子进程变为孤儿进程，防止其成为僵尸进程
            if ((pid = fork()) < 0) {
                errExit("fork");
            } else if (pid > 0)
                //子进程立即终止
                exit(0);
            else {//子子进程留下来处理客户端的通信
                close(lfd);
                handleRequest handle(cfd);
                handle.run();
            }
        }
    }
}


#pragma clang diagnostic pop