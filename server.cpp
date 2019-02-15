#include "server.hpp"
#include <iostream>
#include <thread>
//#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "utility.hpp"
//kqueue
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#define LISTEN_MAX_NUM 32
#define MAXLINE 1024

using namespace std;
string str_compile;

#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
    #define MAXLINE 1024
    #define OPEN_MAX 100
    #define LISTENQ 20
    #define SERV_PORT 5000
    #define INFTIM 1000

    int create_server(int port)
    {
        int i, maxi, listenfd, connfd, sockfd, epfd, nfds, portnumber;
        ssize_t n;
        char line[MAXLINE];
        socklen_t clilen;

        if( (portnumber = port) < 0 )
        {
            fprintf(stderr,"port: %d/a/n", port);
            return 1;
        }

        //声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件
        struct epoll_event ev, events[20];

        //生成epoll专用的文件描述符
        epfd = epoll_create(256);
        struct sockaddr_in clientaddr;
        struct sockaddr_in serveraddr;
        listenfd = socket(AF_INET, SOCK_STREAM, 0);

        //把socket设置为非阻塞方式
        setnonblocking(listenfd);

        //设置与要处理的事件相关的文件描述符
        ev.data.fd = listenfd;

        //设置要处理的事件类型
        ev.events = EPOLLIN|EPOLLET;
        //ev.events=EPOLLIN;

        //注册epoll事件
        epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

       //设置服务端地址结构
        bzero(&serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        char local_addr[] = "127.0.0.1";
        inet_aton(local_addr, &(serveraddr.sin_addr));
        serveraddr.sin_port = htons(portnumber);

        bind(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr));

        listen(listenfd, LISTENQ);

        maxi = 0;
        for( ; ; ) 
        {
            //等待epoll事件的发生
            nfds=epoll_wait(epfd, events, 20, 500);

            //处理所发生的所有事件
            for(i=0;i<nfds;++i)
            {
                if(events[i].data.fd==listenfd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
                {
                    connfd = accept(listenfd, (sockaddr *)&clientaddr, &clilen);
                    if(connfd<0){
                        perror("connfd<0");
                        exit(1);
                    }

                    setnonblocking(connfd);
                    char *str = inet_ntoa(clientaddr.sin_addr);
                    cout << "accapt a connection from " << str << endl;

                    //设置用于读操作的文件描述符
                    ev.data.fd = connfd;
                    //设置用于注测的读操作事件
                    ev.events = EPOLLIN|EPOLLET;
                    //ev.events=EPOLLIN;
                    //注册ev
                    epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
                }
                else if(events[i].events & EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。
                {
                    cout << "EPOLLIN" << endl;
                    if ( (sockfd = events[i].data.fd) < 0) continue;

                    if ( (n = read(sockfd, line, MAXLINE)) < 0) {
                        if (errno == ECONNRESET) 
                        {
                            close(sockfd);
                            events[i].data.fd = -1;
                        } 
                        else
                        {
                            std::cout<<"readline error"<<std::endl;
                        }
                    } 
                    else if (n == 0) 
                    {
                        close(sockfd);
                        events[i].data.fd = -1;
                    }
                    line[n] = '\0';
                    cout << "read " << line << endl;
                    //设置用于写操作的文件描述符
                    ev.data.fd = sockfd;
                    //设置用于注测的写操作事件
                    ev.events = EPOLLOUT|EPOLLET;
                    //修改sockfd上要处理的事件为EPOLLOUT
                    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
                }
                else if(events[i].events & EPOLLOUT) // 如果有数据发送
                {
                    sockfd = events[i].data.fd;
                    write(sockfd, line, n);
                    //设置用于读操作的文件描述符
                    ev.data.fd = sockfd;
                    //设置用于注测的读操作事件
                    ev.events = EPOLLIN|EPOLLET;
                    //修改sockfd上要处理的事件为EPOLIN
                    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
                }
            }
        }
        return 0;
    }
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
        //将事件注册到kqueue
        void RegisterEvent(int kq, int fd, int filters, int flags)
        {
            struct kevent changes[1];
            EV_SET(&changes[0], fd, filters, flags, 0, 0, NULL);
            int res = kevent(kq, changes, 1, NULL, 0, NULL);
            IF_EXIT(res < 0, "kevent");
        }

        int create_server(int port)
        {
            int listenfd, connfd, sockfd, n_ready, n_read, res;
            struct sockaddr_in clientaddr;
            struct sockaddr_in serveraddr;
            socklen_t clilen;
            char line[MAXLINE];

            int kq = kqueue(); // kqueue对象
            IF_EXIT(kq <= 0, "kqueue");
         
            // kqueue的事件结构体
            struct kevent events[LISTEN_MAX_NUM]; // kevent返回的事件列表

            //1. 创建监听套接字
            listenfd = socket(AF_INET, SOCK_STREAM, 0);
            IF_EXIT(listenfd <= 0, "socket");

            setreuseaddr(listenfd);

            //2. 注册读事件
            RegisterEvent(kq, listenfd, EVFILT_READ, EV_ADD | EV_ENABLE);

            //3. 设置服务端地址结构
            bzero(&serveraddr, sizeof(serveraddr));
            serveraddr.sin_family = AF_INET;
            serveraddr.sin_addr.s_addr = INADDR_ANY;
            serveraddr.sin_port = htons(port);

            //4. 绑定监听套接字
            res = ::bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
            IF_EXIT(res < 0, "bind");

            //5. 开始监听
            cout << "start listen" << endl;
            res = listen(listenfd, LISTEN_MAX_NUM);
            IF_EXIT(res < 0, "listen");

            for( ; ; )
            {
                //等待事件
                cout << "wait for event" << endl;
                n_ready = kevent(kq, NULL, 0, events, LISTEN_MAX_NUM, NULL);
                IF_EXIT(n_ready <= 0, "kevent");

                for(int i = 0; i < n_ready; ++i)
                {
                    struct kevent event = events[i];
                    int sockfd = event.ident;
                    if(event.flags & EV_ERROR || event.flags & EV_EOF)
                    {
                        cerr << "event error." << endl;
                        close(sockfd);
                        continue;
                    }
                    if(sockfd == listenfd && event.filter == EVFILT_READ)
                    {
                        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
                        IF_EXIT(connfd < 0, "accept");

                        setnonblocking(connfd);
                        char *str = inet_ntoa(clientaddr.sin_addr);
                        cout << "accapt a connection from " << str << endl;

                        //注册读事件，并enable it
                        RegisterEvent(kq, connfd, EVFILT_READ, EV_ADD | EV_ENABLE);
                        //注册写事件，disbale it
                        RegisterEvent(kq, connfd, EVFILT_WRITE, EV_ADD | EV_DISABLE);
                    }
                    else if(event.filter == EVFILT_READ)
                    {
                        n_read = read(sockfd, line, MAXLINE);
                        cout << "n_read = " << n_read << endl;
                        line[n_read] = '\0';
                        cout << "recv from client: " << line << endl;

                        //disable读事件，防止一直读
                        RegisterEvent(kq, sockfd, EVFILT_READ, EV_ADD | EV_DISABLE);
                        //enable写事件，准备回复客户端
                        RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE);
                        cout << "debug 1" << endl;
                    }
                    else if(event.filter == EVFILT_WRITE)
                    {
                        cout << "debug 2" << endl;
                        write(sockfd, line, strlen(line));
                        //disable写事件，防止一直写
                        RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_ADD | EV_DISABLE);
                        //enable读事件，准备从客户端读消息
                        RegisterEvent(kq, sockfd, EVFILT_READ, EV_ADD | EV_ENABLE);
                    }
                    else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception, deleted from kqueue." << endl;
                        close(sockfd);
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
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
    #error "Unknown compiler"
#endif

struct TaskData
{
    TaskData(int n): i(n){}
    int i;
};

void test_taskque()
{
    CTaskQueue<shared_ptr<TaskData>> que;

    std::thread producer([&que](){
        for(int i = 0; i < 100; ++i)
        {
            que.AddTask(make_shared<TaskData>(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::thread consumer_1([&que](){
        while(1)
        {
            cout << "consumer_1 wait\n";
            shared_ptr<TaskData> pTask = que.Wait_GetTask();
            cout << "consumer_1 get\n" << endl;
            cout << pTask->i << '\n';
        }
    });

    std::thread consumer_2([&que](){
        while(1)
        {
            cout << "consumer_2 wait\n";
            shared_ptr<TaskData> pTask = que.Wait_GetTask();
            cout << "consumer_2 get\n";
            cout << pTask->i << '\n';
        }
    });

    producer.join();
    consumer_1.join();
    consumer_2.join();
}

int main()
{
    int port;
    cout << "input port: ";
    cin >> port;
    create_server(port);

    return 0;
}