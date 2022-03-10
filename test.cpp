//
// Created by 17336 on 2022/2/28.
//

#include <mutex>
#include <iostream>
#include <thread>
#include <math.h>
#include <vector>
#include "timer.h"

using namespace std;

void func() {}

int main() {

    int efd;
    if ((efd = epoll_create1(0)) == -1) {
        exit(1);
    }
    timerHeap p(efd);
    clientData *cdata=new clientData[10];
    vector<time_t> t{6,5,2,7,6,9,4,5,2,1};
    void (timerHeap::*ptr)(const clientData &)=&timerHeap::disconnect;
    for (int i = 0; i < 10; ++i) {
        cdata[i].fd=i;
        p.pushTimer(myTimer(t[i],ptr,cdata[i]));
    }
    for (int i = 0; i < 10; ++i) {
        std::cout<<p.heap[0].expireTime<<',';
        p.popTimer();
    }
}

