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
#include <mutex>


#define TIMEOUT 1000

class myTimer;

class timerHeap;

//每个客户所维持的信息
struct clientData {
    int fd;
    sockaddr_in addr;

    clientData() {}

    clientData(int fd, const sockaddr_in &addr) : fd(fd), addr(addr) {}
};

//定时器到期时决定调用哪一个处理函数
enum class callWhich {
    disconnect,
    reconnect,
};

class myTimer {
public:
    //定时器过期的绝对时间
    time_t expireTime;

    //定时器过期时的回调函数
    callWhich func;

    //用户数据
    clientData userData;

    myTimer() {}

    myTimer(time_t delay, callWhich func_, int fd_, sockaddr_in addr_) : expireTime(time(nullptr) + delay), func(func_),
                                                                         userData(fd_, addr_) {
    }

    myTimer(time_t delay, callWhich func_, clientData cd) : expireTime(time(nullptr) + delay), func(func_),
                                                                         userData(cd) {
    }

    virtual ~myTimer() {
    }
};


class timerHeap {
private:
    //时间堆同步锁
    std::mutex mut;
    //根据fd找到其对应timer
    std::unordered_map<int, int> map;
    //epoll fd
    int efd;
    //堆
    std::vector<myTimer> heap;
public:

    timerHeap(int efd_) : efd(efd_), heap(0) {

    }

    virtual ~timerHeap() {
    }

    bool disconnect(const clientData &p) {
        if (map.count(p.fd)) return false;
        return erase(map[p.fd]);
    }


    //定时时间到，开始处理不活跃连接
    time_t tick() {
        std::lock_guard<std::mutex> lock(mut);
        int len = heap.size(), i = 0;
        //从堆顶一直处理完所有已超时的定时器
        while (i < len && heap[i].expireTime <= time(nullptr)) {
            //根据不同的定时器决定断开连接还是重新连接
            switch (heap[i].func) {
                case callWhich::disconnect:
                    disconnect(heap[i].userData);
            }
            ++i;
        }
        if (heap.empty()) return TIMEOUT;
        return heap[0].expireTime - time(nullptr);
    }

    //获取堆顶结点超时时间
    time_t getLatestTime() {
        std::lock_guard<std::mutex> lock(mut);
        if (heap.empty()) return TIMEOUT;
        return heap[0].expireTime - time(nullptr);
    }

    /*对堆的每一次操作都影响着map[fd]的heap下标值*/

    void pushTimer(myTimer t) {
        std::lock_guard<std::mutex> lock(mut);
        /*有可能在push的时候，堆里已经有fd对应的节点了(有可能fd在线程中因读写错误而被close，但fd定时器并没删除)
         * 这时候要先删除在push*/
        int fd = t.userData.fd;
        if (map.count(fd)) {
            int temp = map[fd];
            erase(temp);
        }
        heap.push_back(std::move(t));
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
        std::lock_guard<std::mutex> lock(mut);
        erase(0);
    }

    bool delayTimer(int fd, time_t delay) {
        std::lock_guard<std::mutex> lock(mut);
        if (!map.count(fd)) return false;
        int i = map[fd];
        heap[i].expireTime += delay;
        adjustHeap(i);
        return true;
    }

    //当fd出错时，外界使用此接口删除定时器(同时提供了关闭连接、清除epoll fd等)
    bool eraseFd(int fd) {
        std::lock_guard<std::mutex> lock(mut);
        if (map.count(fd)) return false;
        return erase(map[fd]);
    }

private:
    //辅助函数

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
