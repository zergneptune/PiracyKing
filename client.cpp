#include "client.hpp"
#include <iostream>
#include <sys/socket.h>
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

using namespace std;

#if 0
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
#endif

//将事件注册到kqueue
void RegisterEvent(int kq, int fd, int filters, int flags)
{
    struct kevent changes[1];
    EV_SET(&changes[0], fd, filters, flags, 0, 0, NULL);
    kevent(kq, changes, 1, NULL, 0, NULL);
}

int connect_server(const char* server_ip, int server_port)
{
	int connfd;
	char text[1024];
	struct sockaddr_in serveraddr;

	// kqueue的事件结构体
    struct kevent events[LISTEN_MAX_NUM]; // kevent返回的事件列表

	//创建连接套接字
	cout << "create connfd socket" << endl;
    connfd = socket(AF_INET, SOCK_STREAM, 0);
	if(connfd == -1)
	{
		perror("socket");
		return 0;
	}

    //创建kqueue
	int kq = kqueue(); // kqueue对象
    if(kq < 0)
    {
        perror("kqueue");
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

    //注册读事件
    RegisterEvent(kq, connfd, EVFILT_READ, EV_ADD | EV_ENABLE);

    for( ; ;)
    {
		cout << "enter:" << endl;
    	gets(text);
    	write(connfd, text, 1024);

    	//等待事件
        n_ready = kevent(kq, NULL, 0, events, LISTEN_MAX_NUM, NULL);
    	for(int i = 0; i < n_ready; ++i)
    	{
    		struct kevent event = events[i];
            int sockfd = event.ident;

            if(event.flags & EV_ERROR)
            {
                cerr << "event error." << endl;
                continue;
            }
            
            if(event.filter & EVFILT_READ)
            {
                n_read = read(sockfd, line, MAXLINE);
                line[n_read] = '\0';

                //注册写事件
                RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE);
            }
    	}
    }
}

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
