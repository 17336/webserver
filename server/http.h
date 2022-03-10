//
// Created by 17336 on 2022/3/7.
//

#ifndef WEBSERVER_HTTP_H
#define WEBSERVER_HTTP_H

#include <string>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sstream>

#define MAXLINE 128

class http {

public:
    static bool
    clientError(int fd, const char *pathname, const char *statusCode, const char *statusMsg, const char *longMsg) {
        std::stringstream ss, body;
        body << "<html><title>出现错误" << pathname << longMsg << "</title></html>";
        body.seekg(0, body.end);
        int len = body.tellg();
        ss << "HTTP/1.0 " << statusCode << ' ' << statusMsg << "\r\n";
        ss << "Content-Type: text/html\r\n";
        ss << "Content-Length: " << len << "\r\n\r\n";
        if (write(fd, ss.str().data(), ss.str().size()) == -1) {
            perror("错误响应报文头 出错");
            return false;
        }
        if (write(fd, body.str().data(), body.str().size()) == -1) {
            perror("错误响应报文体 出错");
            return false;
        }
        return true;
    }

    static bool readAndSend(char *s, int fd) {
        //找到method后面的空格并改为NULL字符
        char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
        sscanf(s, "%s %s %s", method, uri, version);
        //如果method不是GET，则发生错误
        if (strcmp(method, "GET") != 0) {
            return clientError(fd, "", "405", "Method not allowed", "未实现的方法");
        }
        //尝试将url标识的文件打开
        std::string fType;
        if (uri[1] == '\0')
            strncpy(uri, "/index.html\0", 12);
        if (strstr(uri, "html") != nullptr)
            fType = "text/html";
        else if (strstr(uri, "png"))
            fType = "image/png";
        else if (strstr(uri, "mp4"))
            fType = "video/x-mpg";
        else fType = "application/octet-stream";
        std::string path("../resources");
        path.append(uri);
        std::fstream is(path, std::fstream::in);
        if (!is.is_open())
            return clientError(fd, uri, "404", "Not found", "无效的资源");

        //获取文件大小
        is.seekg(0, is.end);
        int flength = is.tellg();
        is.seekg(0, is.beg);
        char buf[flength];
        is.read(buf, flength);
        std::stringstream ss;
        ss << "HTTP/1.0 200 OK\r\nContent-Type: " << fType << "\r\nContent-Length: " << flength << "\r\n\r\n";
        if (write(fd, ss.str().data(), ss.str().size()) == -1) {
            perror("响应报文 报文头出错");
            return false;
        }
        if (write(fd, buf, flength) == -1) {
            perror("响应报文体出错");
            return false;
        }
        return true;
    }
};

#endif //WEBSERVER_HTTP_H
