#include "client.hpp"
#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

using namespace std;

int connect_to_server(char* server_ip, int server_port)
{
	int connfd, epfd;
	char text[1024];
	struct sockaddr_in serveraddr;

	//创建连接套接字
    connfd = socket(AF_INET, SOCK_STREAM, 0);
    setnonblocking(connfd)

    //生成epoll专用的文件描述符
    epfd = epoll_create(5);

    //设置服务端地址结构
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_aton(server_ip, &(serveraddr.sin_addr));
    serveraddr.sin_port = htons(server_port);

    //连接
    connect(connfd, (struct sockaddr*)serveraddr, 0);

    struct epoll_event ev, events[5];
    ev.data.fd = connfd;
    ev.data.events |= EPOLLIN|EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

    for( ; ;)
    {
    	gets(text);
    	write(connfd, text, 1024);

    	int n_ready = epoll_wait(epfd, events, 5, 500);
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