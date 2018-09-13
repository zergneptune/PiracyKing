#include "server.hpp"
#include <iostream>
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
#define LISTEN_MAX_NUM 10
#define MAXLINE 1024

using namespace std;

#if 0
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
#endif

//将事件注册到kqueue
void RegisterEvent(int kq, int fd, int filters, int flags)
{
    struct kevent changes[1];
    EV_SET(&changes[0], fd, filters, flags, 0, 0, NULL);
    int res = kevent(kq, changes, 1, NULL, 0, NULL);
    if(res < 0)
    {
        perror("kevent");
        exit(1);
    }
}

int create_server(int port)
{
    cout << "port: " << port << endl;
    int listenfd, connfd, sockfd, n_ready, n_read, res;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    socklen_t clilen;

    char line[MAXLINE];

    if( port < 0 )
    {
        fprintf(stderr,"port: %d\n", port);
        return 1;
    }

    int kq = kqueue(); // kqueue对象
    if(kq < 0)
    {
        perror("kqueue");
        return 0;
    }
 
    // kqueue的事件结构体
    struct kevent events[LISTEN_MAX_NUM]; // kevent返回的事件列表

    //1. 创建监听套接字
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
    {
        perror("socket");
        exit(1);
    }

    setreuseaddr(listenfd);

    //2. 注册读事件
    RegisterEvent(kq, listenfd, EVFILT_READ, EV_ADD | EV_ENABLE);

    //3. 设置服务端地址结构
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    //char local_addr[] = "127.0.0.1";
    //inet_aton(local_addr, &(serveraddr.sin_addr));
    serveraddr.sin_addr.s_addr = htonl( INADDR_ANY ); 
    serveraddr.sin_port = htons(port);

    //4. 绑定监听套接字
    bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    cout << "errno: " << errno << endl;

    //5. 开始监听
    cout << "start listen" << endl;
    res = listen(listenfd, LISTEN_MAX_NUM);
    if(res < 0)
    {
        perror("listen");
        exit(1);
    }

    for( ; ; )
    {
        //等待事件
        cout << "wait for event" << endl;
        n_ready = kevent(kq, NULL, 0, events, LISTEN_MAX_NUM, NULL);
        if(n_ready <= 0)
        {
            perror("kevent");
            return 0;
        }

        for(int i = 0; i < n_ready; ++i)
        {
            struct kevent event = events[i];
            int sockfd = *((int*)event.udata);
            /*******************************/
            int sockfd_2 = event.ident;

            if(event.flags & EV_ERROR)
            {
                cerr << "event error." << endl;
                continue;
            }

            if(sockfd == listenfd && event.filter & EVFILT_READ)
            {
                connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
                if(connfd<0){
                    perror("connfd<0");
                    exit(1);
                }

                setnonblocking(connfd);
                char *str = inet_ntoa(clientaddr.sin_addr);
                cout << "accapt a connection from " << str << endl;

                //注册读事件
                RegisterEvent(kq, sockfd, EVFILT_READ, EV_ADD | EV_ENABLE);
            }
            else if(event.filter & EVFILT_READ)
            {
                n_read = read(sockfd, line, MAXLINE);
                line[n_read] = '\0';

                //注册写事件
                RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE);
            }
            else if(event.filter & EVFILT_WRITE)
            {
                write(sockfd, line, n_read);
                //注销写事件，防止一直写
                RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_DISABLE);
            }
            else if(event.filter & EVFILT_EXCEPT)
            {
                cerr << "event exception, deleted from kqueue." << endl;
                RegisterEvent(kq, sockfd, EVFILT_READ | EVFILT_WRITE, EV_DELETE);
            }
        }
    }

}

int main()
{
    int port;
    cout << "input port: ";
    cin >> port;
    create_server(port);
    return 1;
}
