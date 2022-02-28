//
// Created by 17336 on 2022/2/26.
//

#include "inet_sockets.h"
#include "deal_error.h"

int main(int argc,char **argv){
    int fd;
    if((fd= inetConnect("101.35.51.210","60200",SOCK_STREAM))<0)
        exit(0);
    char buf[50]="123";
    if(send(fd,buf,13,0)<0)
        errExit("send");
    if(recv(fd,buf,4,0)<0)
        errExit("recv");
    std::cout<<buf<<std::endl;
}
