#include "client.hpp"
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "utility.hpp"

using namespace std;

#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        //define something for Windows (64-bit only)
    #else
        //define something for Windows (32-bit only)
    #endif
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
         // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
        //kqueue
        #include <sys/types.h>
        #include <sys/event.h>
        #include <sys/time.h>
        #define LISTEN_MAX_NUM 10

        //将事件注册到kqueue
        void RegisterEvent(int kq, int fd, int filters, int flags)
        {
            struct kevent changes[1];
            EV_SET(&changes[0], fd, filters, flags, 0, 0, NULL);
            int res = kevent(kq, changes, 1, NULL, 0, NULL);
            IF_EXIT(res < 0, "kevent");
        }

        int connect_server(const char* server_ip, int server_port)
        {
            int connfd;
            char line[1024];
            int n_ready, n_read = 0;
            struct sockaddr_in serveraddr;

            // kqueue的事件结构体
            struct kevent events[LISTEN_MAX_NUM]; // kevent返回的事件列表

            //创建连接套接字
            cout << "create connfd socket" << endl;
            connfd = socket(AF_INET, SOCK_STREAM, 0);
            IF_EXIT(connfd < 0, "socket");

            //创建kqueue
            int kq = kqueue(); // kqueue对象
            IF_EXIT(kq < 0, "kqueue");

            //设置服务端地址结构
            cout << "set serveraddr: " << server_ip << endl;
            cout << "port: " << server_port << endl;
            bzero(&serveraddr, sizeof(serveraddr));
            serveraddr.sin_family = AF_INET;
            serveraddr.sin_addr.s_addr = inet_addr(server_ip);
            serveraddr.sin_port = htons(server_port);

            //连接
            cout << "connect" << endl;
            int res = connect(connfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
            IF_EXIT(res < 0, "connect");

            //设置为非阻塞
            setnonblocking(connfd);

            //注册读事件
            RegisterEvent(kq, connfd, EVFILT_READ, EV_ADD | EV_ENABLE);

            for( ; ;)
            {
                cout << "enter:" << endl;
                cin.ignore(numeric_limits<std::streamsize>::max(), '\n');
                cin.get(line, 1024, '\n');
                res = write(connfd, line, strlen(line));
                IF_EXIT(res < 0, "write");

                //等待事件
                n_ready = kevent(kq, NULL, 0, events, LISTEN_MAX_NUM, NULL);
                IF_EXIT(n_ready < 0, "kevent");
                for(int i = 0; i < n_ready; ++i)
                {
                    struct kevent event = events[i];
                    int sockfd = event.ident;

                    if(event.flags & EV_ERROR || event.flags & EV_EOF)
                    {
                        cerr << "event error." << endl;
                        continue;
                    }
                    
                    if(event.filter == EVFILT_READ)
                    {
                        n_read = read(sockfd, line, 1024);
                        line[n_read] = '\0';
                        cout << "recv from server: " << line << endl;
                    }
                    else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception, deleted from kqueue." << endl;
                        RegisterEvent(kq, sockfd, EVFILT_READ, EV_DELETE);
                        RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_DELETE);
                    }
                }
            }
        }
    #else
        #error "Unknown Apple platform"
    #endif
#elif __ANDROID__
    // android
#elif __linux__
    // linux
    #include <sys/epoll.h>

    int connect_server(const char* server_ip, int server_port)
    {
        int connfd, epfd;
        char text[1024];
        struct sockaddr_in serveraddr;

        //创建连接套接字
        cout << "create connfd socket" << endl;
        connfd = socket(AF_INET, SOCK_STREAM, 0);
        if(connfd == -1)
        {
            perror("connfd");
        }

        //生成epoll专用的文件描述符
        cout << "create epfd" << endl;
        epfd = epoll_create(5);
        if(epfd == -1)
        {
            perror("epfd");
            return 0;
        }

        //设置服务端地址结构
        cout << "set serveraddr" << endl;
        bzero(&serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        inet_aton(server_ip, &(serveraddr.sin_addr));
        serveraddr.sin_port = htons(server_port);

        //连接
        cout << "connect" << endl;
        int res = connect(connfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
        if(res != 0)
        {
            perror("connect");
            return 0;
        }

        //设置为非阻塞
        setnonblocking(connfd);

        struct epoll_event ev, events[5];
        ev.data.fd = connfd;
        ev.events |= EPOLLIN|EPOLLET;
        cout << "add connfd to epfd" << endl;
        epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

        for( ; ;)
        {
            cout << "enter:" << endl;
            gets(text);
            write(connfd, text, 1024);

            int n_ready = epoll_wait(epfd, events, 5, 500);
            cout << "n_ready = " << n_ready << endl;
            for(int i = 0; i < n_ready; ++i)
            {
                if(events[i].events & EPOLLIN)
                {
                    read(events[i].data.fd, text, 1024);
                    cout << "read from server " << text << endl;
                }
            }
        }
    }
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
    #error "Unknown compiler"
#endif

int main()
{
	string IP;
	int port = 0;
	std::cout<< "请输入ip:";
	cin>>IP;
	std::cout<< "请输入port:";
	cin>>port;
	connect_server(IP.c_str(), port);
	return 0;
}
