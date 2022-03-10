//
// Created by 17336 on 2022/3/5.
//

#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

#include <atomic>
#include <functional>
#include <thread>
#include "joinThreads.h"
#include "threadsafe_queue.h"
#include <signal.h>

class threadPool {
    //是否要终止所有线程（出故障或者析构时）
    std::atomic_bool done;
    //存储任务的线程安全队列:任务是执行一个空返回值空参数的函数（如果想给任务添加参数，可以定义一个可调用对象，对象成员变量作为参数）
    threadsafe_queue<std::function<void()>> workQueue;
    //线程组
    std::vector<std::thread> threads;
    //析构时等待所有未执行完毕的线程（如果不等待，所有joinable但没执行完的线程会被terminate）
    joinThreads joiner;

    //线程池中的线程一经初始化立刻进入此函数
    void workThread() {
        //线程一经初始化必须立即屏蔽SIGPIPE信号
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set,SIGPIPE);
        if(pthread_sigmask(SIG_BLOCK,&set, nullptr)!=0){
            errExit("pthread_sigmask");
        }
        while (!done) {
            //用来存储从任务队列中取出的任务
            std::function<void(void)> task;
//            //使用waitAndPop等待工作队列可用
//            workQueue.waitAndPop(task);
            //使用try_pop+yield比直接使用waitAndPop要好，因为如果正在waitAndPop的时候done=true了，
            // try_pop做法的线程会直接终止不在取任务
            if (workQueue.try_pop(task)) {
                task();
            } else {
                //休息一下，同时下次调度重新冲while(!done)开始
                std::this_thread::yield();
            }
        }
    }

public:
    //按定义顺序构造，这样threads在joiner前构造
    threadPool() : done(false), joiner(threads) {
        unsigned long numOfThread = 8;
        try {
            for (unsigned long i = 0; i < numOfThread; ++i) {
                threads.push_back(std::thread(&threadPool::workThread, this));
            }
        }
        catch (...) {
            //线程池里只要有一个坏线程，立刻终止所有线程并抛出异常
            done = true;
            throw;
        }
    }

    //线程池析构时依次执行：done=true、析构joiner、threads、workQueue
    ~threadPool() {
        done = true;
    }

    //像这种函数参数有很多种（函数指针、函数对象等等），直接一个模板函数+模板类型推断完事
    template<typename Func>
    void pushTask(Func f) {
        workQueue.push(std::function<void()> (f));
    }


};


#endif //WEBSERVER_THREADPOOL_H
