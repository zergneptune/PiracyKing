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
#include <string.h>

using namespace std;

void setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock, F_SETFL, opts)<0)
    {
        perror("fcntl(sock, SETFL, opts)");
        exit(1);
    }
}

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
