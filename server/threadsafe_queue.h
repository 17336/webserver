//
// Created by 17336 on 2022/3/5.
//

#ifndef WEBSERVER_THREADSAFE_QUEUE_H
#define WEBSERVER_THREADSAFE_QUEUE_H

#include <queue>
#include <exception>
#include <mutex>
#include <memory>
#include <condition_variable>

struct emptyQueue : std::exception {
    const char *what() const noexcept;
};

/**
 * 线程安全队列要保证所有对原始队列的操作同步
 * @tparam T
 */
template<typename T>
class threadsafe_queue {
private:
    std::queue<T> data;
    mutable std::mutex m;
    std::condition_variable dataCond;
public:
    threadsafe_queue() {}

    //拷贝另一个安全队列
    threadsafe_queue(const threadsafe_queue &other) {
        std::lock_guard<std::mutex> lock(other.m);
        data = other.data;
    }

    threadsafe_queue &operator=(const threadsafe_queue &) = delete;

    void waitAndPop(T &value) {
        std::unique_lock<std::mutex> lock(m);
        //等待队列非空时在进行pop
        dataCond.wait(lock, [this]() { return !this->empty(); });
        value=std::move(data.front());
        data.pop();
    }

    bool try_pop(T &value){
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) return false;
        value=std::move(data.front());
        data.pop();
        return true;
    }

    void push(T value) {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(value));
        dataCond.notify_one();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }

};


#endif //WEBSERVER_THREADSAFE_QUEUE_H
