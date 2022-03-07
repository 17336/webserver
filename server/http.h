//
// Created by 17336 on 2022/3/7.
//

#ifndef WEBSERVER_HTTP_H
#define WEBSERVER_HTTP_H

#include <string>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sstream>

class http {

public:
    static bool
    clientError(int fd, const char *pathname, const char *statusCode, const char *statusMsg, const char *longMsg) {
        std::stringstream ss, body;
        body << "<html><title>出现错误" << pathname << longMsg << "</title></html>";
        body.seekg(0, body.end);
        int len = body.tellg();
        ss << "HTTP/1.0 " << statusCode << ' ' << statusMsg << "\r\n";
        ss << "Content-Type: text/html";
        ss << "Content-Length: " << len;
        if (write(fd, ss.str().data(), ss.str().size()) == -1) {
            perror("write");
            return false;
        }
        if (write(fd, body.str().data(), body.str().size()) == -1) {
            perror("write");
            return false;
        }
        return true;
    }

    static bool readAndSend(char *s, int fd) {
        //找到method后面的空格并改为NULL字符
        char *end = strchr(s, ' ');
        *end = '\0';
        //如果method不是GET，则发生错误
        if (strcmp(s, "GET") != 0) {
            return clientError(fd, "", "405", "Method not allowed", "未实现的方法");
        }
        //尝试将url标识的文件打开
        ++end;
        *(strchr(s, ' ')) = '\0';
        std::string fType;
        if (strstr(end, "html") != nullptr)
            fType = "text/html";
        else fType = "image/png";
        std::string path("../resources");
        path.append(end);
        std::fstream is(path, std::fstream::in);
        if (!is.is_open())
            return clientError(fd, end, "404", "Not found", "无效的资源");

        //获取文件大小
        is.seekg(0, is.end);
        int flength = is.tellg();
        is.seekg(0, is.beg);
        char buf[flength];
        is.read(buf, flength);
        std::stringstream ss;
        ss << "HTTP/1.0 200 OK\r\nContent-Type: " << fType << "\r\nContent-Length: " << flength;
        if (write(fd, ss.str().data(), ss.str().size()) == -1) {
            perror("write");
            return false;
        }
        if (write(fd, buf, flength) == -1) {
            perror("write");
            return false;
        }
        return true;
    }
};

#endif //WEBSERVER_HTTP_H
