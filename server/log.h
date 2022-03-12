//
// Created by 17336 on 2022/3/12.
//

#ifndef WEBSERVER_LOG_H
#define WEBSERVER_LOG_H

#include <fstream>
#include <queue>
#include <sys/time.h>
#include <condition_variable>
#include <mutex>

std::string getCurDay() {
    time_t t = time(nullptr);
    tm *m= localtime(&t);
    return std::to_string(m->tm_year+1900)+ std::to_string(m->tm_mon+1)+ std::to_string(m->tm_mday);
}

//log遵循单例模式
class log {
    //构造函数私有化
    log() {}

    //当前日志操作的文件名
    std::string filename;
    //日志所在目录
    std::string path;
    //日志文件能写入的最大行数
    int maxWriteLine;
    //当前已写入行数
    int curWriteLine;
    //日志文件一天一个，记录当前日志文件的日期
    std::string curDate;
    //操作文件的流对象
    std::fstream file;
    //阻塞队列，保存等待写入的日志信息
    std::queue<std::string> q;
    //工作线程
    std::thread::id tid;

    //用于日志队列的条件变量
    std::condition_variable con;
    std::mutex mut;
public:
    //提供一个全局访问点,懒汉+局部static
    static log *getInstance() {
        static log instance;
        return &instance;
    }

    //初始化日志
    bool init(const char *path_, const char *filename_= nullptr, int maxline = 5000000) {
        curWriteLine = 0;
        maxWriteLine = maxline;

        path = path_;

        curDate=getCurDay();

        if (filename_ == nullptr) {
            filename=curDate;
        }
        file.open(path + '/' + filename,std::fstream::app);
        if (!file.is_open()) return false;

        file<<std::endl;
        //让线程执行log唯一实例的work函数
        std::thread ted(&log::work, getInstance());

        return true;
    }

    //日志线程的处理逻辑：等待队列中有要写的信息，然后输入到file中
    void work() {
        std::unique_lock<std::mutex> lock(mut);
        //因为只有一个日志线程等待条件变量，所以不需要while循环
        con.wait(lock, [this] { return !q.empty(); });
        //写日志
        file << q.front() << std::endl;
        q.pop();
        lock.unlock();
        ++curWriteLine;

        //如果时间过了一天，则重开一个日志文件
        std::string date=getCurDay();
        if (date!=curDate) {
            filename = curDate;
            file.open(path + '/' + filename);
            curWriteLine = 0;
        }
        //日志文件写满了，也重开文件
        if (curWriteLine > maxWriteLine) {
            filename += "_1";
            file.open(path + '/' + filename);
            curWriteLine = 0;
        }
    }


    void push(std::string s) {
        std::lock_guard<std::mutex> lock(mut);
        q.push(std::move(s));
        con.notify_one();
    }
};

#endif //WEBSERVER_LOG_H
