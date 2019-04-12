#include "server.hpp"
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iterator>
#include <json/json.h>
#include "game.hpp"

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
        #define LISTEN_MAX_NUM 1024
        #define MAXLINE 1024

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
                    /*else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception, deleted from kqueue." << endl;
                        close(sockfd);
                    }*/
                }
            }
        }

        void CServerMng::listen_thread_func(int port)
        {
            int connfd, sockfd, n_ready;
            struct sockaddr_in clientaddr;
            struct sockaddr_in serveraddr;
            socklen_t clilen;
            struct kevent events[LISTEN_MAX_NUM];

            int kq = kqueue();
            IF_EXIT(kq <= 0, "kqueue");
         
            int listenfd = socket(AF_INET, SOCK_STREAM, 0);
            IF_EXIT(listenfd <= 0, "socket");

            setreuseaddr(listenfd);

            bzero(&serveraddr, sizeof(serveraddr));
            serveraddr.sin_family = AF_INET;
            serveraddr.sin_addr.s_addr = INADDR_ANY;
            serveraddr.sin_port = htons(port);

            int res = ::bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
            IF_EXIT(res < 0, "bind");

            res = listen(listenfd, LISTEN_MAX_NUM);
            IF_EXIT(res < 0, "listen");

            RegisterEvent(kq, listenfd, EVFILT_READ, EV_ADD | EV_ENABLE);

            while(1)
            {
                n_ready = kevent(kq, NULL, 0, events, LISTEN_MAX_NUM, NULL);
                IF_EXIT(n_ready <= 0, "kevent");

                struct kevent event = events[0];
                sockfd = event.ident;
                if(event.flags & EV_ERROR || event.flags & EV_EOF)
                {
                    cerr << "event error, close it." << endl;
                    close(sockfd);
                    continue;
                }

                if(event.filter == EVFILT_READ)
                {
                    connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
                    IF_EXIT(connfd < 0, "accept");

                    setnonblocking(connfd);
                    char *str = inet_ntoa(clientaddr.sin_addr);
                    cout << "有新的连接, ip: " << str << endl;

                    //加入队列
                    m_queSockFD.AddTask(TSocketFD(connfd));
                }
                /*else if(event.filter == EVFILT_EXCEPT)
                {
                    cerr << "event exception, close it." << endl;
                    close(sockfd);
                }*/
            }
        }

        void CServerMng::socket_recv_thread_func()
        {
            int n_ready = 0;
            int n_ret = 0;
            struct kevent events[LISTEN_MAX_NUM];
            TTaskData t_task;
            TSocketFD t_sockfd;

            int kq = kqueue();
            IF_EXIT(kq <= 0, "kqueue");

            auto deleter = [](char* p){
                if( p != NULL)
                {
                    delete[] p;
                }};

            while(1)
            {
                struct timespec timeout = { 0, 10 * 1000000};
                //等待事件
                n_ready = kevent(kq, NULL, 0, events, LISTEN_MAX_NUM, &timeout);
                IF_EXIT(n_ready < 0, "kevent");
                for(int i = 0; i < n_ready; ++i)
                {
                    struct kevent event = events[i];
                    int sockfd = event.ident;

                    if(event.flags & EV_ERROR || event.flags & EV_EOF)
                    {
                        //cerr << "event error, close it." << endl;
                        struct sockaddr_in clientaddr;
                        socklen_t addrlen;
                        getpeername(sockfd, (struct sockaddr*)&clientaddr, &addrlen);
                        char* ip = inet_ntoa(clientaddr.sin_addr);
                        short port = ntohs(clientaddr.sin_port);
                        printf("客户端 %s:%d 掉线.\n", ip, port);
                        int nClientId = m_COnlinePlayers.remove_player_by_socketfd(sockfd);
                        m_pGameServer->remove_player(nClientId);
                        close(sockfd);
                        continue;
                    }
                    
                    if(event.filter == EVFILT_READ)
                    {
                        std::shared_ptr<char> pBuffer(new char[sizeof(TMsgHead)], deleter);
                        n_ret = readn(sockfd, pBuffer.get(), sizeof(TMsgHead));
                        if(n_ret < 0)
                        {
                            cout << "读sockfd失败！errno = " << errno << endl;
                            close(sockfd);
                            continue;
                        }

                        TMsgHead* pMsgHead = reinterpret_cast<TMsgHead*>(pBuffer.get());
                        uint64_t  nMsgId   = pMsgHead->nMsgId;
                        size_t    szMsgLen = pMsgHead->szMsgLength;
                        MsgType   msgtype = pMsgHead->msgType;
                        pBuffer.reset(new char[szMsgLen + 1], deleter);
                        memset(pBuffer.get(), 0, szMsgLen + 1);
                        n_ret = readn(sockfd, pBuffer.get(), szMsgLen);
                        if(n_ret < 0)
                        {
                            cout << "读sockfd失败！errno = " << errno << endl;
                            close(sockfd);
                            continue;
                        }

                        //json解码msg，放入任务队列
                        cout << "接收到客户端命令: " << msgtype << endl;
                        m_queTaskData.AddTask(std::make_shared<TTaskData>(
                                                    nMsgId,
                                                    sockfd,
                                                    msgtype,
                                                    string(pBuffer.get())));
                    }
                    /*else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception." << endl;
                        close(sockfd);
                        continue;
                    }*/
                }

                //处理连接套接字
                while(m_queSockFD.Try_GetTask(t_sockfd))
                {
                    //注册读事件
                    RegisterEvent(kq, t_sockfd.sockfd, EVFILT_READ, EV_ADD | EV_ENABLE);
                }
            }
        }

        void CServerMng::socket_send_thread_func()
        {
            int nRet = 0;
            int nSendBufferSize = 0;
            char* pSendBuffer = NULL;
            TMsgHead msgHead;
            while(1)
            {
                shared_ptr<TTaskData> pTask = m_queSendMsg.Wait_GetTask();
                msgHead.nMsgId = pTask->nTaskId;
                msgHead.msgType = pTask->msgType;
                msgHead.szMsgLength = pTask->strMsg.size();
                nSendBufferSize = sizeof(TMsgHead) + msgHead.szMsgLength;
                pSendBuffer = new char[nSendBufferSize];
                memset(pSendBuffer, 0, sizeof(nSendBufferSize));
                memcpy(pSendBuffer, &msgHead, sizeof(TMsgHead));
                memcpy(pSendBuffer + sizeof(TMsgHead), pTask->strMsg.c_str(), msgHead.szMsgLength);
                std::cout << "服务器返回：" << pTask->msgType << std::endl;
                nRet = writen(pTask->nSockfd, pSendBuffer, nSendBufferSize);
                if(nRet < 0)
                {
                    cout << "发送线程：发送数据失败！" << endl;
                }

                if(pSendBuffer != NULL)
                {
                    delete[] pSendBuffer;
                    pSendBuffer = NULL;
                }
            }
        }

        void CServerMng::send_muticast()
        {
            int res = 0;
            int sockfd = 0;
            struct sockaddr_in mcast_addr;
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            IF_EXIT(sockfd < 0, "socket");

            memset(&mcast_addr, 0, sizeof(mcast_addr));
            mcast_addr.sin_family = AF_INET;
            mcast_addr.sin_addr.s_addr = inet_addr("224.0.0.100");/*设置多播IP地址*/
            mcast_addr.sin_port = htons(10010);/*设置多播端口*/

            char msg[] = "muticast message";
            while(1)
            {
                res = sendto(sockfd,
                    msg,
                    sizeof(msg),
                    0,
                    (struct sockaddr*)(&mcast_addr),
                    sizeof(mcast_addr));

                std::cout << "res = " << res << endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        void CServerMng::send_broadcast()
        {
            int res = 0;
            int sockfd = 0;
            struct sockaddr_in bcast_addr;
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            IF_EXIT(sockfd < 0, "socket");

            memset(&bcast_addr, 0, sizeof(bcast_addr));
            bcast_addr.sin_family = AF_INET;
            bcast_addr.sin_addr.s_addr = inet_addr("192.168.2.255");
            bcast_addr.sin_port = htons(10000);

            //设置套接字广播属性
            int opt = 1;
            res = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

            char msg[] = "broadcast message";
            while(1)
            {
                res = sendto(sockfd,
                    msg,
                    sizeof(msg),
                    0,
                    (struct sockaddr*)(&bcast_addr),
                    sizeof(bcast_addr));

                std::cout << "res = " << res << endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
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

CClientInfoMng::CClientInfoMng(){}

CClientInfoMng::~CClientInfoMng(){}

bool CClientInfoMng::add_client(int cid, TClientInfo clientinfo)
{
    std::lock_guard<std::mutex>  lck(m_mtx);
    auto res_pair = m_mapClientInfo.insert(
        std::make_pair(
            cid,
            std::make_shared<TClientInfo>(clientinfo)
            )
        );

    return res_pair.second;
}

int CClientInfoMng::get_max_client_id()
{
    std::lock_guard<std::mutex>  lck(m_mtx);
    if(m_mapClientInfo.empty())
    {
        return 0;
    }
    else
    {
        auto iter = m_mapClientInfo.begin();
        std::advance(iter, m_mapClientInfo.size() - 1);
        return iter->first;
    }
}

bool CClientInfoMng::is_client_existed(std::string& account)
{
    std::lock_guard<std::mutex>  lck(m_mtx);
    for(auto iter = m_mapClientInfo.begin(); iter != m_mapClientInfo.end(); ++ iter)
    {
        if(iter->second->strAccount == account)
        {
            return true;
        }
    }

    return false;
}

std::shared_ptr<TClientInfo> CClientInfoMng::find_client(std::string& account)
{
    std::lock_guard<std::mutex>  lck(m_mtx);
    for(auto iter = m_mapClientInfo.begin(); iter != m_mapClientInfo.end(); ++ iter)
    {
        if(iter->second->strAccount == account)
        {
            return iter->second;
        }
    }

    return std::shared_ptr<TClientInfo>();
}

std::string CClientInfoMng::get_name(int cid)
{
    std::lock_guard<std::mutex>  lck(m_mtx);
    auto iter = m_mapClientInfo.find(cid);
    if(iter != m_mapClientInfo.end())
    {
        return iter->second->strName;
    }

    return string("");
}

COnlinePlayers::COnlinePlayers(){}

COnlinePlayers::~COnlinePlayers(){}

void COnlinePlayers::add_player(SocketFd fd, ClientID cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    m_mapOlinePlayers.insert(std::make_pair(fd, cid));
}

int COnlinePlayers::remove_player_by_socketfd(SocketFd fd)
{
    int nClientId = -1;
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapOlinePlayers.find(fd);
    if(iter != m_mapOlinePlayers.end())
    {
        nClientId = iter->second;
        m_mapOlinePlayers.erase(iter);
    }

    return nClientId;
}

void COnlinePlayers::remove_player_by_clientid(ClientID id)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    for(auto iter = m_mapOlinePlayers.begin();
        iter != m_mapOlinePlayers.end();
        ++ iter)
    {
        if(iter->second == id)
        {
            m_mapOlinePlayers.erase(iter);
            return;
        }
    }
}

bool COnlinePlayers::is_player_online(ClientID cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    for(auto iter = m_mapOlinePlayers.begin();
        iter != m_mapOlinePlayers.end();
        ++ iter)
    {
        if(iter->second == cid)
        {
            return true;
        }
    }

    return false;
}

int COnlinePlayers::get_sockfd(ClientID cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    for(auto iter = m_mapOlinePlayers.begin();
        iter != m_mapOlinePlayers.end();
        ++ iter)
    {
        if(iter->second == cid)
        {
            return iter->first;
        }
    }

    return -1;
}

CServerMng::CServerMng()
{
    m_pGameServer = new CGameServer();
}

CServerMng::~CServerMng(){}

void CServerMng::start(int port)
{
    /*
    ** 添加测试账号
    ** acounts: 123, passwd: 123, name: luffy
    ** acounts: 456, passwd: 456, name: zoro
    ** acounts: 789, passwd: 789, name: sanji
    */
    m_ClientInfoMng.add_client(1,
                    TClientInfo(
                        1,
                        string("123"),
                        string("123"),
                        string("luffy")));
    m_ClientInfoMng.add_client(2,
                    TClientInfo(
                        2,
                        string("456"),
                        string("456"),
                        string("zoro")));
    m_ClientInfoMng.add_client(3,
                    TClientInfo(
                        3,
                        string("789"),
                        string("789"),
                        string("sanji")));
    /**************************************************/
    std::thread listenThread([this, port]()
        {
            this->listen_thread_func(port);
        });

    m_nPort = port;
    cout << "服务启动，监听端口号：" << port << endl;
    listenThread.join();
}

void CServerMng::init()
{
    init_thread();
    cout << "线程初始化成功." << endl;
}

void CServerMng::init_thread()
{
    std::thread socketRecvThread([this]()
        {
            this->socket_recv_thread_func();
        });


    std::thread socketSendThread([this]()
        {
            this->socket_send_thread_func();
        });


    std::thread taskProcThread([this]()
        {
            this->task_proc_thread_func();
        });

    socketRecvThread.detach();
    socketSendThread.detach();
    taskProcThread.detach();
}

void CServerMng::task_proc_thread_func()
{
    while(1)
    {
        shared_ptr<TTaskData> pTask = m_queTaskData.Wait_GetTask();
        TTaskData task;
        switch(pTask->msgType)
        {
            case MsgType::HEARTBEAT:
                cout << "处理客户端心跳." << endl;
                break;
            case MsgType::REGIST:
                cout << "处理客户端注册请求." << endl;
                do_regiser(pTask);
                break;
            case MsgType::LOGIN:
                cout << "处理客户端登录请求." << endl;
                do_login(pTask);
                break;
            case MsgType::LOGOUT:
                cout << "处理客户端登出请求." << endl;
                do_logout(pTask);
                break;
            case MsgType::CHAT:
                cout << "处理客户端聊天请求." << endl;
                break;
            case MsgType::QUERY_ROOM:
                cout << "处理客户端查询房间列表请求." << endl;
                m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::QUERY_ROOM_RSP,
                    m_pGameServer->get_gameid_list()));
                break;
            case MsgType::QUERY_ROOM_PLAYERS:
                cout << "处理客户端查询玩家列表请求." << endl;
                do_query_room_players(pTask);
                break;
            case MsgType::CREATE_ROOM:
                cout << "处理客户端创建房间请求." << endl;
                do_create_room(pTask);
                break;
            case MsgType::JOIN_ROOM:
                cout << "处理客户端加入房间请求." << endl;
                do_join_room(pTask);
                break;
            case MsgType::QUIT_ROOM:
                cout << "处理客户端退出房间请求." << endl;
                do_quit_room(pTask);
                break;
            case MsgType::GAME_READY:
                cout << "处理客户端游戏准备请求." << endl;
                do_game_ready(pTask);
                break;
            case MsgType::QUIT_GAME_READY:
                cout << "处理客户端退出游戏准备请求." << endl;
                do_quit_game_ready(pTask);
                break;
            case MsgType::REQ_GAME_START:
                cout << "处理客户端请求开始游戏." << endl;
                do_request_game_start(pTask);
                break;
            case MsgType::GAME_START:
                cout << "处理客户端开始游戏." << endl;
                do_game_start(pTask);
                break;
            default:
                break;
        }
    }
}

void CServerMng::do_regiser(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    bool bRes = false;
    std::string strAccount;
    std::string strPasswd;
    std::string strName;
    if(reader.parse(pTask->strMsg, root))
    {
        strAccount = root["account"].asString();
        strPasswd = root["passwd"].asString();
        strName = root["name"].asString();
        root.clear();
        bRes = m_ClientInfoMng.is_client_existed(strAccount);
        if(bRes)
        {
            root["res"] = -1;
            root["msg"] = string("账号重复.");
        }
        else
        {
            int nNewClientID = m_ClientInfoMng.get_max_client_id() + 1;
            m_ClientInfoMng.add_client(nNewClientID,
                                    TClientInfo(nNewClientID,
                                                strAccount,
                                                strPasswd,
                                                strName));
            root["res"] = 0;
            root["cid"] = nNewClientID;
        }
    }
    else
    {
        root.clear();
        root["res"] = -2;
        root["msg"] = string("json解码失败.");
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::REGIST_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_login(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    bool bRes = false;
    std::string strAccount;
    std::string strPasswd;
    std::string strName;
    if(reader.parse(pTask->strMsg, root))
    {
        strAccount = root["account"].asString();
        strPasswd = root["passwd"].asString();
        root.clear();
        auto pClientInfo = m_ClientInfoMng.find_client(strAccount);
        if(pClientInfo && pClientInfo->strPasswd == strPasswd)
        {
            bRes = m_COnlinePlayers.is_player_online(pClientInfo->nClientID);
            if(bRes)
            {
                root["res"] = -1;
                root["msg"] = string("你已经登陆了.");
            }
            else
            {
                m_COnlinePlayers.add_player(pTask->nSockfd, pClientInfo->nClientID);
                root["res"] = 0;
                root["cid"] = pClientInfo->nClientID;
            }
        }
        else
        {
            root["res"] = -1;
            root["msg"] = string("账号或密码错误.");
        }
    }
    else
    {
        root.clear();
        root["res"] = -2;
        root["msg"] = string("json解码失败.");
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::LOGIN_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_logout(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    if(reader.parse(pTask->strMsg, root))
    {
        int nCid = root["cid"].asInt();
        root.clear();
        m_COnlinePlayers.remove_player_by_clientid(nCid);
        root["res"] = 0;
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::LOGOUT_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_create_room(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    if(reader.parse(pTask->strMsg, root))
    {
        int cid = root["cid"].asInt();
        std::string strRoomName = root["room_name"].asString();
        root.clear();
        uint64_t gid = m_pGameServer->create_game(cid, strRoomName);
        if(gid < 0)
        {
            root["res"] = -1;
            if(gid == -1)
            {
                root["msg"] = string("房间名已经存在.");
            }
            else
            {
                root["msg"] = string("未知错误.");
            }
        }
        else
        {
            root["res"] = 0;
            root["gid"] = gid;
            root["gname"] = strRoomName;
            root["room_owner"] = cid;
        }
    }
    else
    {
        root.clear();
        root["res"] = -2;
        root["msg"] = string("json解码失败.");
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::CREATE_ROOM_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_query_room_players(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    std::vector<int> vecCids;
    Json::Value player_list;
    if(reader.parse(pTask->strMsg, root))
    {
        uint64_t gid = root["gid"].asUInt64();
        int room_owner = m_pGameServer->get_room_owner(gid);
        m_pGameServer->get_cid_list(gid, vecCids);
        Json::Value value;
        for(auto iter = vecCids.begin(); iter != vecCids.end(); ++ iter)
        {
            value["cid"] = *iter;
            value["name"] = m_ClientInfoMng.get_name(*iter);
            player_list.append(value);
        }

        root.clear();
        root["room_owner"] = room_owner;
        root["client_info"] = player_list;
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::QUERY_ROOM_PLAYERS_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_join_room(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    if(reader.parse(pTask->strMsg, root))
    {
        uint64_t gid = root["gid"].asUInt64();
        int cid = root["cid"].asInt();
        root.clear();
        int nRes = m_pGameServer->add_game_player(gid, cid);
        if(nRes < 0)
        {
            if(nRes == -1)
            {
                root["res"] = -1;
                root["msg"] = string("超出房间人数限制(2人).");
            }
            else if(nRes == -2)
            {
                root["res"] = -2;
                root["msg"] = string("游戏房间不存在.");
            }
            
        }
        else
        {
            root["res"] = 0;
        }
    }
    else
    {
        root.clear();
        root["res"] = -3;
        root["msg"] = string("json解码失败.");
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::JOIN_ROOM_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_quit_room(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    if(reader.parse(pTask->strMsg, root))
    {
        uint64_t gid = root["gid"].asUInt64();
        int cid = root["cid"].asInt();
        root.clear();
        m_pGameServer->remove_player(gid, cid);
        root["res"] = 0;
    }
    else
    {
        root.clear();
        root["res"] = -2;
        root["msg"] = string("json解码失败.");
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::QUIT_ROOM_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_game_ready(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    if(reader.parse(pTask->strMsg, root))
    {
        uint64_t gid = root["gid"].asUInt64();
        int cid = root["cid"].asInt();
        root.clear();
        int nRes = m_pGameServer->game_ready(gid, cid);
        if(nRes < 0)
        {
            root["res"] = -1;
            root["msg"] = string("未知错误.");
        }
        else
        {
            root["res"] = 0;
        }
    }
    else
    {
        root.clear();
        root["res"] = -2;
        root["msg"] = string("json解码失败.");
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::GAME_READY_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_quit_game_ready(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    if(reader.parse(pTask->strMsg, root))
    {
        uint64_t gid = root["gid"].asUInt64();
        int cid = root["cid"].asInt();
        root.clear();
        int nRes = m_pGameServer->quit_game_ready(gid, cid);
        if(nRes < 0)
        {
            root["res"] = -1;
            root["msg"] = string("未知错误.");
        }
        else
        {
            root["res"] = 0;
        }
    }
    else
    {
        root.clear();
        root["res"] = -2;
        root["msg"] = string("json解码失败.");
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::QUIT_GAME_READY_RSP,
                    fwriter.write(root)));
}

void CServerMng::do_game_start(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    std::vector<int> vecCids;
    if(reader.parse(pTask->strMsg, root))
    {
        uint64_t gid = root["gid"].asUInt64();
        m_pGameServer->get_cid_list(gid, vecCids);
        root.clear();
        root["res"] = 0;
        int nRandSeed = time(NULL);
        root["rand_seed"] = nRandSeed;

        m_pGameServer->game_start(gid);
    }
    else
    {
        root.clear();
        root["res"] = -2;
        root["msg"] = string("json解码失败.");
    }

    //通知该游戏房间下的所有玩家开始游戏
    for(auto& cid : vecCids)
    {
        int sockfd = m_COnlinePlayers.get_sockfd(cid);
        if(sockfd > 0)
        {
            m_queSendMsg.AddTask(make_shared<TTaskData>(
                    0,
                    sockfd,
                    MsgType::GAME_START,
                    fwriter.write(root)));
        }
    }
}

void CServerMng::do_request_game_start(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    Json::Reader reader;
    if(reader.parse(pTask->strMsg, root))
    {
        uint64_t gid = root["gid"].asUInt64();
        root.clear();
        int nPlayerNums = m_pGameServer->get_player_nums(gid);
        bool bIsGameReady = m_pGameServer->get_game_ready_status(gid);
        if(nPlayerNums < 2)
        {
            root["res"] = -1;
            root["msg"] = string("房间人数未满.");
        }
        else if(!bIsGameReady)
        {
            root["res"] = -1;
            root["msg"] = string("请等待所有玩家准备.");
        }
        else
        {
            root["res"] = 0;
        }
    }
    else
    {
        root.clear();
        root["res"] = -2;
        root["msg"] = string("json解码失败.");
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(
                    pTask->nTaskId,
                    pTask->nSockfd,
                    MsgType::REQ_GAME_START,
                    fwriter.write(root)));
}

int main()
{
    /*int port;
    cout << "input port: ";
    cin >> port;
    create_server(port);*/

    int port = 10086;
    //cout << "输入监听端口号: ";
    //port = get_input_number();

    CServerMng serverMng;

    serverMng.init();

    serverMng.start(port);
    
    return 0;
}