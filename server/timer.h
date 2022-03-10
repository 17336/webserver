//
// Created by 17336 on 2022/3/9.
//

#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H

#include <netinet/in.h>
#include <vector>
#include <algorithm>
#include <ctime>
#include <unordered_map>
#include <deque>
#include <sys/epoll.h>
#include <unistd.h>

class myTimer;

class timerHeap;

//每个客户所维持的信息
struct clientData {
    int fd;
    sockaddr_in addr;
};

class myTimer {
public:
    //定时器过期的绝对时间
    time_t expireTime;

    //定时器过期时的回调函数
    void (timerHeap::*callWhenExpire)(const clientData &);

    //用户数据
    clientData userData;

    myTimer() {}

    myTimer(time_t delay, void(timerHeap::*func)(const clientData &), clientData data) : expireTime(time(nullptr) + delay),
                                                                                    callWhenExpire(func),
                                                                                    userData(data) {

    }

    virtual ~myTimer() {
    }
};


class timerHeap {
private:
    //根据fd找到其对应timer
    std::unordered_map<int, int> map;
    //epoll fd
    int efd;
public:
    //堆
    std::vector<myTimer> heap;

    timerHeap(int efd_) : efd(efd_), heap(0) {

    }

    virtual ~timerHeap() {
    }

    void disconnect(const clientData &p) {
        erase(map[p.fd]);
    }


    //定时时间到，开始处理不活跃连接
    time_t tick() {
        int len = heap.size();
        for (int i = 0; i < len; ++i) {
            if (heap[i].expireTime <= time(nullptr)) {
                (this->*(heap[i].callWhenExpire))(heap[i].userData);
            }
        }
        return heap[0].expireTime;
    }


    /*对堆的每一次操作都影响着map[fd]的heap下标值*/

    void pushTimer(myTimer t) {
        /*有可能在push的时候，堆里已经有fd对应的节点了(有可能fd在线程中因读写错误而被close，但fd定时器并没删除)
         * 这时候要先删除在push*/
        int fd = t.userData.fd;
        if (map.count(fd)) {
            int temp = map[fd];
            erase(temp);
        }
        heap.push_back(t);
        //插入后从插入位置（堆尾）向上调整
        int i = heap.size() - 1, fa = (i - 1) / 2;
        map[t.userData.fd] = i;
        while (i > 0 && heap[i].expireTime < heap[fa].expireTime) {
            mySwap(i, fa);
            i = fa;
            fa = (i - 1) / 2;
        }
    }

    void popTimer() {
        erase(0);
    }

    //从堆中删除i结点需要做的事：关闭fd，释放clientdata、timer、从epoll实例中删除fd
    bool erase(int i) {
        int len = heap.size();
        if (i >= len) return false;
        int fd = heap[i].userData.fd;
        mySwap(i, len - 1);
        epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        map.erase(fd);
        heap.pop_back();
        adjustHeap(i);
        return true;
    }

    void delayTimer(int i, time_t delay) {
        heap[i].expireTime += delay;
        adjustHeap(i);
    }

    //从结点i开始向下调整堆
    void adjustHeap(int i) {
        //当左孩子超出堆，停止
        int len = heap.size();
        int l = 2 * i + 1, r = l + 1, m;
        if (l >= len) return;
        if (r >= len) m = l;
        else m = heap[l].expireTime < heap[r].expireTime ? l : r;
        //如果根大于孩子，为了维持小根堆，交换之
        if (heap[i].expireTime > heap[m].expireTime) {
            mySwap(i, m);
        }
        adjustHeap(m);
    }

    //因为哈希表中存的是堆中下标值，交换堆中两节点时一定要记得更新哈希表中结点下标
    void mySwap(int i, int m) {
        int fd1 = heap[i].userData.fd, fd2 = heap[m].userData.fd;
        std::swap(heap[i], heap[m]);
        map[fd1] = m;
        map[fd2] = i;
    }
};

#endif //WEBSERVER_TIMER_H
