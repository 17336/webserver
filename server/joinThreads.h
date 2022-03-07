//
// Created by 17336 on 2022/3/5.
//

#ifndef WEBSERVER_JOINTHREADS_H
#define WEBSERVER_JOINTHREADS_H

#include <vector>
#include <thread>

/**
 * 将此类作为成员变量的线程池在析构时，将会等待所有线程结束才被销毁
 */
class joinThreads {
    std::vector<std::thread> &threads;
public:
    explicit joinThreads(std::vector<std::thread> &threads):threads(threads){}
    ~joinThreads(){
        for (unsigned long i = 0; i < threads.size(); ++i) {
            if(threads[i].joinable()) threads[i].join();
        }
    }
};


#endif //WEBSERVER_JOINTHREADS_H
