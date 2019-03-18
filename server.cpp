#include "server.hpp"
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
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
                    else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception, deleted from kqueue." << endl;
                        close(sockfd);
                    }
                }
            }
        }

        CListenThrFunc::CListenThrFunc(SOCKETFD_QUE* pQueSockFD): m_pQueSockFD(pQueSockFD){}
        CListenThrFunc::~CListenThrFunc(){}

        void CListenThrFunc::operator()(int port)
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
                    m_pQueSockFD->AddTask(TSocketFD(connfd));
                }
                else if(event.filter == EVFILT_EXCEPT)
                {
                    cerr << "event exception, close it." << endl;
                    close(sockfd);
                }
            }
        }

        CSocketSend::CSocketSend(TASK_QUE* p): m_pQueSendMsg(p){}

        CSocketSend::~CSocketSend(){}

        void CSocketSend::operator()()
        {
            int sockfd = 0;
            int nRet = 0;
            int nSendBufferSize = 0;
            char* pSendBuffer = NULL;
            TMsgHead msgHead;
            while(1)
            {
                shared_ptr<TTaskData> pTask = m_pQueSendMsg->Wait_GetTask();
                sockfd = pTask->nSockfd;
                msgHead.msgType = pTask->msgType;
                msgHead.szMsgLength = pTask->strMsg.size();
                nSendBufferSize = sizeof(TMsgHead) + msgHead.szMsgLength;
                pSendBuffer = new char[nSendBufferSize];
                memset(pSendBuffer, 0, sizeof(nSendBufferSize));
                memcpy(pSendBuffer, &msgHead, sizeof(TMsgHead));
                memcpy(pSendBuffer + sizeof(TMsgHead), pTask->strMsg.c_str(), msgHead.szMsgLength);
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

        CSocketRecv::CSocketRecv(SOCKETFD_QUE* pQueSockFD, TASK_QUE* pTaskData): 
            m_pQueSockFD(pQueSockFD), m_pQueTaskData(pTaskData){}

        CSocketRecv::~CSocketRecv(){}

        void CSocketRecv::operator()()
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
                        cerr << "event error, close it." << endl;
                        struct sockaddr_in clientaddr;
                        socklen_t addrlen;
                        getpeername(sockfd, (struct sockaddr*)&clientaddr, &addrlen);
                        char* ip = inet_ntoa(clientaddr.sin_addr);
                        short port = ntohs(clientaddr.sin_port);
                        printf("客户端 %s:%d 掉线.\n", ip, port);
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
                        cout << "接收到客户端的信息: " << pBuffer << endl;
                        m_pQueTaskData->AddTask(std::make_shared<TTaskData>(
                                                    string("123"),  //从msg中得到任务id
                                                    sockfd,
                                                    msgtype,
                                                    string(pBuffer.get())));
                    }
                    else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception." << endl;
                        close(sockfd);
                        continue;
                    }
                }

                //处理连接套接字
                while(m_pQueSockFD->Try_GetTask(t_sockfd))
                {
                    //注册读事件
                    RegisterEvent(kq, t_sockfd.sockfd, EVFILT_READ, EV_ADD | EV_ENABLE);
                }
            }
        }

        CTaskProc::CTaskProc(TASK_QUE* pQueSend, TASK_QUE* pQueTask): m_pQueSendMsg(pQueSend), m_pQueTaskData(pQueTask){}
        CTaskProc::~CTaskProc(){}
        void CTaskProc::operator()()
        {
            while(1)
            {
                shared_ptr<TTaskData> pTask = m_pQueTaskData->Wait_GetTask();
                TTaskData task;
                switch(pTask->msgType)
                {
                    case HEARTBEAT:
                        cout << "处理客户端心跳." << endl;
                        break;
                    case REGIST:
                        cout << "处理客户端注册请求." << endl;
                        m_pQueSendMsg->AddTask(make_shared<TTaskData>(
                            string("123"),  //从msg中得到任务id
                            pTask->nSockfd,
                            REGIST_RSP,
                            "regist success."));
                        break;
                    case LOGIN:
                        cout << "处理客户端登录请求." << endl;
                        m_pQueSendMsg->AddTask(make_shared<TTaskData>(
                            string("123"),  //从msg中得到任务id
                            pTask->nSockfd,
                            LOGIN_RSP,
                            "login success."));
                        break;
                    case LOGOUT:
                        cout << "处理客户端登出请求." << endl;
                        break;
                    case CHAT:
                        cout << "处理客户端聊天请求." << endl;
                        break;
                    default:
                        break;
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

CServerMng::CServerMng(): 
m_socketRecv(&m_queSockFD, &m_queTaskData),
m_socketSend(&m_queSendMsg),
m_taskProc(&m_queSendMsg, &m_queTaskData),
m_listenThrFunc(&m_queSockFD)
{}

CServerMng::~CServerMng(){}

void CServerMng::start(int port)
{
    std::thread listenThread(m_listenThrFunc, port);
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
    std::thread socketRecvThread(m_socketRecv);
    std::thread socketSendThread(m_socketSend);
    std::thread taskProcThread(m_taskProc);

    socketRecvThread.detach();
    socketSendThread.detach();
    taskProcThread.detach();
}

int main()
{
    /*int port;
    cout << "input port: ";
    cin >> port;
    create_server(port);*/

    /*int port = 0;
    cout << "输入监听端口号: ";
    port = get_input_number();

    CServerMng serverMng;

    serverMng.init();

    serverMng.start(port);*/

    CServerMng serverMng;

    serverMng.send_broadcast();
    
    return 0;
}