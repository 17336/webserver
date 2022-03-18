//
// Created by 17336 on 2022/2/24.
//

#include "inet_sockets.h"
#include "../server/log.h"

/**
 * 本函数用于客户端创建socket描述符并将其连接到某服务器上:
 *      过程：getaddrinfo->socket->connect
 * @param host 服务器域名/主机名
 * @param service 服务名
 * @param type 有连接还是无连接
 * @return
 */
int inetConnect(const char *host, const char *service, int type) {
    //查找host+service确定的可用的远程套接字地址，并创建对应的套接字描述符
    addrinfo *addr, hint;
    memset(&hint, 0, sizeof(addrinfo));
    hint.ai_addr = nullptr;
    hint.ai_canonname = nullptr;
    hint.ai_next = nullptr;
    hint.ai_socktype = type;
    hint.ai_family = AF_UNSPEC;
    int err = 0;
    if ((err = getaddrinfo(host, service, &hint, &addr)) != 0) {
        std::cout << gai_strerror(err);
        return -1;
    }
    int fd = -1;
    //一个addr socket和connect都成功时才算成功
    while (addr != nullptr) {
        if ((fd = socket(addr->ai_family, type, addr->ai_protocol)) >= 0 &&
            connect(fd, addr->ai_addr, addr->ai_addrlen) == 0)
            break;
        addr = addr->ai_next;
        //socket成功但connect失败重连时，需要将已有fd关闭后重连
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }
    freeaddrinfo(addr);
    return fd;
}


/**
 * 本函数主要用于TCP服务器创建一个可用（指成功绑定并监听到service服务）的套接字描述符
 * @param service 要监听的服务名
 * @param backlog
 * @param addrlen 监听地址结构的长度
 * @return
 */
int inetListen(const char *service, int backlog, socklen_t *addrlen) {
    //查找service确定的可用的本地套接字地址，并创建对应的套接字描述符
    addrinfo *addr, hint;
    memset(&hint, 0, sizeof(addrinfo));
    hint.ai_addr = nullptr;
    hint.ai_canonname = nullptr;
    hint.ai_next = nullptr;
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    int err = 0;
    if ((err = getaddrinfo(nullptr, service, &hint, &addr)) != 0) {
        std::cout << gai_strerror(err);
        return -1;
    }

    //一个addr socket bind listen都成功时才算成功
    int fd = -1;
    while (addr != nullptr) {
        if ((fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) >= 0) {
            int opval = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opval, sizeof(opval)) < 0) {
                perror("setsockopt");
                exit(-1);
            }
        }
        if (bind(fd, addr->ai_addr, addr->ai_addrlen) == 0 && listen(fd, backlog) == 0) {
            if (addrlen != nullptr)
                *addrlen = addr->ai_addrlen;
            log::getInstance()->push("listening");
            break;
        }
        addr = addr->ai_next;
        //如果socket成功了但其他的操作没成功，记得先关掉fd在重新操作
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    freeaddrinfo(addr);
    return fd;
}

/**
 * 此函数主要用于UDP服务器创建一个可用的套接字描述符
 * @param service 套接字绑定服务代表的地址
 * @param addrlen 地址长度
 * @param type 套接字类型
 * @return
 */
int inetBind(const char *service, socklen_t *addrlen, int type) {
    //查找service确定的可用的本地套接字地址，并创建对应的套接字描述符
    addrinfo *addr, hint;
    memset(&hint, 0, sizeof(addrinfo));
    hint.ai_addr = nullptr;
    hint.ai_canonname = nullptr;
    hint.ai_next = nullptr;
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = type;
    hint.ai_flags = AI_PASSIVE;
    int err = 0;
    if ((err = getaddrinfo(nullptr, service, &hint, &addr)) != 0) {
        std::cout << gai_strerror(err);
        return -1;
    }
    //一个addr socket bind listen都成功时才算成功
    int fd = -1;
    while (addr != nullptr) {
        if ((fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) >= 0 &&
            bind(fd, addr->ai_addr, addr->ai_addrlen) == 0) {
            if (addrlen != nullptr)
                *addrlen = addr->ai_addrlen;
            break;
        }
        addr = addr->ai_next;
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    freeaddrinfo(addr);
    return fd;
}