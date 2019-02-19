#include "client.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <thread>
#include <future>

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

                    cout << "sockfd = " << sockfd << ", connfd = " << connfd << endl;

                    if(event.flags & EV_ERROR || event.flags & EV_EOF)
                    {
                        cerr << "event error." << endl;
                        close(sockfd); //记得关闭此错误socket文件描述符
                        //RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_DELETE);
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
                        //RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_DELETE);
                    }
                }
            }
        }

        int CClientMng::connect_server(string ip, int port)
        {
            int connfd;
            struct sockaddr_in serveraddr;

            // kqueue的事件结构体
            struct kevent events[LISTEN_MAX_NUM]; // kevent返回的事件列表

            //创建连接套接字
            connfd = socket(AF_INET, SOCK_STREAM, 0);
            IF_EXIT(connfd < 0, "socket");

            //创建kqueue
            int kq = kqueue(); // kqueue对象
            IF_EXIT(kq < 0, "kqueue");

            //设置服务端地址结构
            bzero(&serveraddr, sizeof(serveraddr));
            serveraddr.sin_family = AF_INET;
            serveraddr.sin_addr.s_addr = inet_addr(ip.c_str());
            serveraddr.sin_port = htons(port);

            //连接
            int res = connect(connfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
            IF_EXIT(res < 0, "connect");

            //设置为非阻塞
            setnonblocking(connfd);

            //保存 kq 和 server sockfd
            m_nKq = kq;
            m_nServerSockfd = connfd;

            return 0;
        }

        CSocketSend::CSocketSend(TASK_QUE* p, int* pKq): m_pQueSendMsg(p), m_pKq(pKq){}

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
                msgHead.szMsgLength = pTask->strMsg.size() + 1;
                nSendBufferSize = sizeof(TMsgHead) + msgHead.szMsgLength + 1;
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

        CSocketRecv::CSocketRecv(int* pKq, int* pFd): m_pKq(pKq), m_pServerSockfd(pFd){}

        CSocketRecv::~CSocketRecv(){}

        void CSocketRecv::operator()()
        {
            int n_ready = 0;
            int n_ret = 0;
            struct kevent events[LISTEN_MAX_NUM];
            //注册读事件
            RegisterEvent(*m_pKq, *m_pServerSockfd, EVFILT_READ, EV_ADD | EV_ENABLE);
            while(1)
            {
                //等待事件
                n_ready = kevent(*m_pKq, NULL, 0, events, LISTEN_MAX_NUM, NULL);
                IF_EXIT(n_ready < 0, "kevent");
                for(int i = 0; i < n_ready; ++i)
                {
                    struct kevent event = events[i];
                    int sockfd = event.ident;

                    if(event.flags & EV_ERROR || event.flags & EV_EOF)
                    {
                        cerr << "event error." << endl;
                        close(sockfd); //记得关闭此错误socket文件描述符
                        continue;
                    }
                    
                    if(event.filter == EVFILT_READ)
                    {
                        char* pBuffer = new char[sizeof(TMsgHead)];
                        n_ret = readn(sockfd, pBuffer, sizeof(TMsgHead));
                        if(n_ret < 0)
                        {
                            cout << "读sockfd失败！" << endl;
                            close(sockfd);
                            continue;
                        }

                        if(pBuffer != NULL)
                        {
                            delete[] pBuffer;
                            pBuffer = NULL;
                        }

                        TMsgHead* pMsgHead = reinterpret_cast<TMsgHead*>(pBuffer);
                        pBuffer = new char[sizeof(pMsgHead->szMsgLength)];
                        n_ret = readn(sockfd, pBuffer, pMsgHead->szMsgLength);
                        if(n_ret < 0)
                        {
                            cout << "读sockfd失败！" << endl;
                            close(sockfd);
                            continue;
                        }

                        //json解码msg，放入任务队列
                        cout << "recv: " << pBuffer << endl;

                        if(pBuffer != NULL)
                        {
                            delete[] pBuffer;
                            pBuffer = NULL;
                        }
                    }
                    else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception." << endl;
                        close(sockfd);
                    }
                }
            }
        }

        CTaskProc::CTaskProc(TASK_QUE* pQueSend, TASK_QUE* pQueTask): m_pQueSendMsg(pQueSend), m_pQueTaskData(pQueTask){}
        CTaskProc::~CTaskProc(){}
        void CTaskProc::operator()()
        {
            shared_ptr<TTaskData> pTask = m_pQueTaskData->Wait_GetTask();
            TTaskData task;
            switch(pTask->msgType)
            {
                case HEARTBEAT:
                    cout << "收到心跳任务." << endl;
                    break;
                case CHAT:
                    cout << "收到聊天任务." << endl;
                    break;
                default:
                    break;
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

CClientMng::CClientMng(): 
    m_socketRecv(&m_nKq, &m_nServerSockfd),
    m_socketSend(&m_queSendMsg, &m_nKq),
    m_taskProc(&m_queSendMsg, &m_queTaskData){}

CClientMng::~CClientMng(){}

void CClientMng::system_start()
{
    while(1)
    {
        printf("1. 登录\n");
        printf("2. 注册\n");
        printf("输入：");

        int nInput = get_input_number();
        int nRet = 0;

        switch(nInput)
        {
            case 1:
                nRet = login();
                if(nRet == 0)
                {
                    cout << "登录成功！" << endl;
                }
                break;

            case 2:
                nRet = regist();
                if(nRet == 0)
                {
                    cout << "注册成功！" << endl;
                }
                break;
        }
    }

}

int CClientMng::regist()
{
    printf("输入账号：");
    string strAccount = get_input_string();
    string strPasswd, strPasswdConfirm;
    while(1)
    {
        printf("输入密码：");
        strPasswd = get_input_string();
        printf("确认密码：");
        strPasswdConfirm = get_input_string();

        if(strPasswd != strPasswdConfirm)
        {
            cout << "两次密码不一致，请重新输入！" << endl;
        }
        else
        {
            break;
        }
    }

    m_queSendMsg.AddTask(make_shared<TTaskData>(m_nServerSockfd,
                                                REGIST,
                                                "regist"));

    return 0;
}

int CClientMng::login()
{
    printf("输入账号：");
    string strAccount = get_input_string();
    printf("输入密码：");
    string strPasswd = get_input_string();

    m_queSendMsg.AddTask(make_shared<TTaskData>(m_nServerSockfd,
                                            LOGIN,
                                            "login"));

    return 0;
}

int CClientMng::logout()
{
    return 0;
}

int CClientMng::init(string ip, int port)
{
    int nRet = 0;
    do
    {
        nRet = this->connect_server(ip, port);
        if(nRet != 0)
        {
            return -1;
        }
        cout << "连接服务器成功." << endl;
        nRet = init_thread();
        if(nRet != 0)
        {
            return -2;
        }
        cout << "线程初始化成功." << endl;
    }while(0);

    return 0;
}

int CClientMng::init_thread()
{
    thread socketRecvThread(m_socketRecv);
    thread socketSendThread(m_socketSend);
    thread taskProcThread(m_taskProc);

    socketRecvThread.detach();
    socketSendThread.detach();
    taskProcThread.detach();
    return 0;
}

int main()
{
	string IP;
	int port = 0;
	std::cout<< "请输入ip:";
	cin>>IP;
	std::cout<< "请输入port:";
	cin>>port;
	//connect_server(IP.c_str(), port);

    CClientMng clientMng;

    clientMng.init(IP, port);

    clientMng.system_start();

	return 0;
}
