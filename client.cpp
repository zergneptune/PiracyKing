#include "client.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <thread>
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
                        continue;
                    }
                    
                    if(event.filter == EVFILT_READ)
                    {
                        n_read = read(sockfd, line, 1024);
                        line[n_read] = '\0';
                        cout << "recv from server: " << line << endl;
                    }
                    /*else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception, deleted from kqueue." << endl;
                        RegisterEvent(kq, sockfd, EVFILT_READ, EV_DELETE);
                        //RegisterEvent(kq, sockfd, EVFILT_WRITE, EV_DELETE);
                    }*/
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

        void CClientMng::socket_send_thread_func()
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

        void CClientMng::socket_recv_thread_func()
        {
            int n_ready = 0;
            int n_ret = 0;
            struct kevent events[LISTEN_MAX_NUM];
            auto deleter = [](char* p){
                if( p != NULL)
                {
                    delete[] p;
                }};

            //注册读事件
            RegisterEvent(m_nKq, m_nServerSockfd, EVFILT_READ, EV_ADD | EV_ENABLE);
            while(1)
            {
                //等待事件
                n_ready = kevent(m_nKq, NULL, 0, events, LISTEN_MAX_NUM, NULL);
                IF_EXIT(n_ready < 0, "kevent");
                for(int i = 0; i < n_ready; ++i)
                {
                    struct kevent event = events[i];
                    int sockfd = event.ident;

                    if(event.flags & EV_ERROR || event.flags & EV_EOF)
                    {
                        //cerr << "event error, close it." << endl;
                        cerr << "\r\n与服务器断开连接!" << endl;
                        close(sockfd); //记得关闭此错误socket文件描述符
                        continue;
                    }
                    
                    if(event.filter == EVFILT_READ)
                    {
                        std::shared_ptr<char> pBuffer(new char[sizeof(TMsgHead)], deleter);
                        n_ret = readn(sockfd, pBuffer.get(), sizeof(TMsgHead));
                        if(n_ret < 0)
                        {
                            cout << "读sockfd失败, errno = " << errno << endl;
                            struct sockaddr_in serveraddr;
                            socklen_t addrlen;
                            getpeername(sockfd, (struct sockaddr*)&serveraddr, &addrlen);
                            close(sockfd);
                            continue;
                        }

                        TMsgHead*   pMsgHead = reinterpret_cast<TMsgHead*>(pBuffer.get());
                        uint64_t    nTaskId  = pMsgHead->nMsgId;
                        size_t      szMsgLen = pMsgHead->szMsgLength;
                        MsgType     msgtype  = pMsgHead->msgType;
                        pBuffer.reset(new char[szMsgLen + 1], deleter);
                        memset(pBuffer.get(), 0, szMsgLen + 1);
                        n_ret = readn(sockfd, pBuffer.get(), szMsgLen);
                        if(n_ret < 0)
                        {
                            cout << "读sockfd失败, errno = " << errno << endl;
                            close(sockfd);
                            continue;
                        }

                        //json解码msg，放入任务队列
                        cout << "接收到服务器消息: " << pBuffer << endl;
                        m_queTaskData.AddTask(std::make_shared<TTaskData>(
                                                        nTaskId,
                                                        sockfd,
                                                        msgtype,
                                                        string(pBuffer.get())));
                    }
                    /*else if(event.filter == EVFILT_EXCEPT)
                    {
                        cerr << "event exception." << endl;
                        close(sockfd);
                    }*/
                }
            }
        }

        void CClientMng::recv_muticast()
        {
            int res = 0;
            int sockfd = 0;
            struct sockaddr_in local_addr;
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            IF_EXIT(sockfd < 0, "socket");

            memset(&local_addr, 0, sizeof(local_addr));
            local_addr.sin_family = AF_INET;
            local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            local_addr.sin_port = htons(10010);

            //绑定
            res = ::bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr));
            IF_EXIT(res < 0, "bind");

            //设置本地回环许可
            int loop = 1;
            res = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
            IF_EXIT(res < 0, "setsockopt");

            //加入多播组
            struct ip_mreq mreq;
            mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.100"); /*多播组IP地址*/
            mreq.imr_interface.s_addr = htonl(INADDR_ANY); /*加入的客服端主机IP地址*/

            //本机加入多播组
            res = setsockopt(sockfd,
                IPPROTO_IP,
                IP_ADD_MEMBERSHIP,
                &mreq,
                sizeof(mreq));
            IF_EXIT(res < 0, "setsockopt");

            char buffer[1024];
            socklen_t srvaddr_len = sizeof(local_addr);
            while(1)
            {
                memset(buffer, 0, 1024);
                res = recvfrom(sockfd,
                    buffer,
                    1024,
                    0,
                    (struct sockaddr*)(&local_addr),
                    &srvaddr_len);

                std::cout << "recvfrom : " << buffer << endl;
            }

            //退出多播组
            res = setsockopt(sockfd,
                IPPROTO_IP,
                IP_DROP_MEMBERSHIP,
                &mreq,
                sizeof(mreq));
            IF_EXIT(res < 0, "setsockopt");
        }

        void CClientMng::recv_broadcast()
        {
            int res = 0;
            int sockfd = 0;
            struct sockaddr_in local_addr;
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            IF_EXIT(sockfd < 0, "socket");

            memset(&local_addr, 0, sizeof(local_addr));
            local_addr.sin_family = AF_INET;
            local_addr.sin_addr.s_addr = inet_addr("192.168.2.255");
            local_addr.sin_port = htons(10000);

            res = ::bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr));
            IF_EXIT(res < 0, "bind");

            char buffer[1024];
            socklen_t srvaddr_len = sizeof(local_addr);
            while(1)
            {
                memset(buffer, 0, 1024);
                res = recvfrom(sockfd,
                    buffer,
                    1024,
                    0,
                    (struct sockaddr*)(&local_addr),
                    &srvaddr_len);

                std::cout << "recvfrom : " << buffer << endl;
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

CClientMng::CClientMng()
{
    m_pGameClient = new CGameClient(&m_queTaskData);
}

CClientMng::~CClientMng(){}

void CClientMng::task_proc_thread_func()
{
    while(1)
    {
        shared_ptr<TTaskData> pTask = m_queTaskData.Wait_GetTask();
        switch(pTask->msgType)
        {
            case MsgType::GAME_PLAYER_CMD:
                send_game_player_cmd(pTask);
                break;
            case MsgType::HEARTBEAT:
                cout << "心跳任务." << endl;
                break;
            case MsgType::REGIST_RSP:
                cout << "注册结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::LOGIN_RSP:
                cout << "登录结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::LOGOUT_RSP:
                cout << "登出结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::CHAT:
                cout << "聊天结果." << endl;
                break;
            case MsgType::QUERY_ROOM_RSP:
                cout << "查询房间列表结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::QUERY_ROOM_PLAYERS_RSP:
                cout << "查询玩家列表结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::CREATE_ROOM_RSP:
                cout << "创建房间结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::JOIN_ROOM_RSP:
                cout << "加入房间结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::QUIT_ROOM_RSP:
                cout << "退出房间结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::GAME_READY_RSP:
                cout << "游戏准备结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::QUIT_GAME_READY_RSP:
                cout << "退出游戏准备结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::REQ_GAME_START:
                cout << "请求开始游戏结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            case MsgType::GAME_START:
                cout << "开始游戏结果." << endl;
                set_start(pTask);
                break;
            case MsgType::QUIT_GAME:
                cout << "退出游戏结果." << endl;
                m_cEventNotice.Notice(
                    pTask->nTaskId,
                    pTask->strMsg);
                break;
            default:
                break;
        }
    }
}

void CClientMng::start_menu()
{
    int nRet = 0;
    while(1)
    {
        login_menu();
        main_menu();
    }
}

int CClientMng::login_menu()
{
    int nInput = 0;
    int nRet = 0;
    while(1)
    {
        printf("\x1b[H\x1b[2J");
        printf("**********************\r\n");
        printf("\t1. 登录\n");
        printf("\t2. 注册\n");
        printf("**********************\r\n");
        printf("\t输入: ");
        nInput = get_input_number();

        switch(nInput)
        {
            case 1:
                nRet = login();
                if(nRet == 0)
                {
                    cout << "登录成功！" << endl;
                    enter_any_key_to_continue();
                    return 0;
                }
                break;
            case 2:
                nRet = regist();
                if(nRet == 0)
                {
                    cout << "注册成功！" << endl;
                    enter_any_key_to_continue();
                }
                break;
            default:
                break;
        }
    }
}

int CClientMng::main_menu()
{
    int nInput = 0;
    int nRet = 0;
    uint64_t nRoomId = 0;
    std::string strRoomName;
    while(1)
    {
        printf("\x1b[H\x1b[2J");
        printf("**********************\r\n");
        printf("\t1. 加入房间\n");
        printf("\t2. 创建房间\n");
        printf("**********************\r\n");
        printf("\t(提示: 输入0登出)\r\n");
        printf("\t输入: ");
        nInput = get_input_number();
        switch(nInput)
        {
            case 0:
                logout();
                return 0;
            case 1:
                nRet = room_list_menu(nRoomId, strRoomName);
                if(nRet == 0)
                {
                    nRet = join_room(nRoomId);
                    if(nRet == 0)
                    {
                        cout << "加入房间成功！" << endl;
                        enter_any_key_to_continue();
                        player_list_menu(nRoomId, strRoomName);
                    }
                }
                break;
            case 2:
                nRet = create_room(nRoomId, strRoomName);
                if(nRet == 0)
                {
                    cout << "创建房间成功！" << endl;
                    enter_any_key_to_continue();
                    player_list_menu(nRoomId, strRoomName);
                }
                break;
        }
    }
}

int CClientMng::room_list_menu(uint64_t& nRoomId, std::string& strRoomName)
{
    Json::Value root;
    Json::Reader reader;
    std::string strRoomList;
    int nInput = 0;
    int nRet = 0;
    int nCnt = 0;
    while(1)
    {
        nRet = query_room_list(strRoomList);
        reader.parse(strRoomList, root);
        nCnt = 0;
        printf("\x1b[H\x1b[2J");
        printf("***********  房间列表  ***********\r\n");
        if(root.isArray())
        {
            for(Json::Value::iterator iter = root.begin();
                iter != root.end();
                ++ iter)
            {
                printf("\t%d. 房间: %s 玩家数: %d\r\n", ++ nCnt,
                                                    (*iter)["gname"].asString().c_str(),
                                                    (*iter)["players"].asInt());
            }
            printf("*********************************\r\n");
        }
        else
        {
            printf("\tempty list\n");
            printf("*********************************\r\n");
            enter_any_key_to_continue();
            return -1;
        }
        printf("\t(提示: 输入0返回, 输入房间序号加入房间, 输入其他数字刷新房间列表)\r\n");
        printf("\t输入: ");
        nInput = get_input_number();
        if(nInput >= 1 && nInput <= nCnt)
        {
            nRoomId = root[nInput - 1]["gid"].asUInt64();
            strRoomName = root[nInput - 1]["gname"].asString();
            return 0;
        }
        else if(nInput == 0)
        {
            return -1;
        }
    }
}

int CClientMng::player_list_menu(uint64_t nRoomId, std::string& strRoomName)
{
    Json::Value root;
    Json::Reader reader;
    std::string strPlayerList;
    int nInput = 0;
    int nRes = 0;
    int nCnt = 0;
    int nRoomOwner = 0;
    bool bIsRoomOwner = false;
    while(1)
    {
        query_room_players(nRoomId, strPlayerList);
        root.clear();
        reader.parse(strPlayerList, root);
        nCnt = 0;
        nRoomOwner = root["room_owner"].asInt();
        if(root["room_owner"] == m_nClientID)
        {
            bIsRoomOwner = true;
        }
        else
        {
            bIsRoomOwner = false;
        }

        Json::Value player_list = root["client_info"];
        printf("\x1b[H\x1b[2J");
        printf("***********  房间: %s  ***********\r\n", strRoomName.c_str());
        if(player_list.isArray())
        {
            for(Json::Value::iterator iter = player_list.begin();
                iter != player_list.end();
                ++ iter)
            {
                if((*iter)["cid"] == nRoomOwner)
                {
                    printf("\t玩家%d: %s <--房主-->\r\n",
                                ++ nCnt,
                                (*iter)["name"].asString().c_str());
                }
                else
                {
                    printf("\t玩家%d: %s\r\n",
                                ++ nCnt,
                                (*iter)["name"].asString().c_str());
                }
            }
            printf("*********************************\r\n");
        }
        else
        {
            printf("\tempty list\n");
            printf("*********************************\r\n");
            enter_any_key_to_continue();
            return -1;
        }

        if(bIsRoomOwner)
        {
            printf("\t(提示: 输入0退出房间, 输入其他数字刷新玩家列表)\r\n");
            printf("\t(您是队长: 输入1开始游戏)\r\n");
        }
        else
        {
            printf("\t(提示: 输入0退出房间, 输入1游戏准备, 输入其他数字刷新玩家列表)\r\n");
        }
        printf("\t输入: ");
        nInput = get_input_number();
        if(bIsRoomOwner)
        {
            if(nInput == 0)
            {
                quit_room(nRoomId);
                return -1;
            }
            else if(nInput == 1)
            {
                nRes = request_start_game(nRoomId);
                if(nRes == 0)
                {
                    game_start(nRoomId);
                }
            }
        }
        else
        {
            if(nInput == 0)
            {
                quit_room(nRoomId);
                return -1;
            }
            else if(nInput == 1)
            {
                printf("\x1b[H\x1b[2J");
                printf("(提示: 输入0退出游戏准备)\r\n");
                nRes = game_ready(nRoomId);
                if(nRes == 0)
                {
                    game_start(nRoomId);
                }
            }
        }
    }
}

int CClientMng::regist()
{
    printf("输入账号: ");
    string strAccount = get_input_string();
    printf("输入姓名: ");
    string strName = get_input_string();
    string strPasswd, strPasswdConfirm;
    while(1)
    {
        printf("输入密码: ");
        strPasswd = get_input_string();
        printf("确认密码: ");
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

    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["account"] = strAccount;
    root["passwd"] = strPasswd;
    root["name"] = strName;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                nSid,
                                m_nServerSockfd,
                                REGIST,
                                fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "注册超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "注册失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }
        else
        {
            m_nClientID = root["cid"].asInt();
        }

        return nRet;
    }

    return -1;
}

int CClientMng::login()
{
    printf("输入账号: ");
    string strAccount = get_input_string();
    printf("输入密码: ");
    string strPasswd = get_input_string();

    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["account"] = strAccount;
    root["passwd"] = strPasswd;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    LOGIN,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "登录超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "登录失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }
        else
        {
            m_nClientID = root["cid"].asInt();
        }

        return nRet;
    }

    return -1;
}

int CClientMng::logout()
{
    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["cid"] = m_nClientID;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    LOGOUT,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "登出超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "登出失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }

        return nRet;
    }

    return 0;
}

int CClientMng::create_room(uint64_t& gid, std::string& strRoomName)
{
    printf("输入要创建的房间名: ");
    strRoomName = get_input_string();
    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["cid"] = m_nClientID;
    root["room_name"] = strRoomName;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    CREATE_ROOM,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "创建房间超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "创建房间失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }
        else
        {
            gid = root["gid"].asUInt64();
            strRoomName = root["gname"].asString();
        }

        return nRet;
    }

    return -1;
}

int CClientMng::quit_room(uint64_t gid)
{
    Json::Value root;
    Json::FastWriter fwriter;
    root["gid"] = gid;
    root["cid"] = m_nClientID;
    uint64_t nSid = m_cSnowFlake.get_sid();
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    QUIT_ROOM,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "退出房间超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "退出房间失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }

        return nRet;
    }

    return -1;
}

int CClientMng::query_room_list(std::string& res)
{
    uint64_t nSid = m_cSnowFlake.get_sid();
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    QUERY_ROOM,
                                    string("")));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "查询房间列表失败！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    res = result;
    return 0;
}

int CClientMng::query_room_players(uint64_t gid, std::string& res)
{
    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["gid"] = gid;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    QUERY_ROOM_PLAYERS,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "查询玩家列表超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    res = result;

    return 0;
}

int CClientMng::join_room(uint64_t gid)
{
    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["gid"] = gid;
    root["cid"] = m_nClientID;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    JOIN_ROOM,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "加入房间超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "加入房间失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }

        return nRet;
    }
    return -1;
}

int CClientMng::game_ready(uint64_t gid)
{
    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["gid"] = gid;
    root["cid"] = m_nClientID;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    GAME_READY,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "游戏准备超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "游戏准备失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }

        nRet = wait_to_start();

        return nRet;
    }
    return -1;
}

int CClientMng::quit_game_ready(uint64_t gid)
{
    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["gid"] = gid;
    root["cid"] = m_nClientID;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    QUIT_GAME_READY,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "退出游戏准备超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "退出游戏准备失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }

        return nRet;
    }
    return -1;
}

int CClientMng::wait_to_start()
{
    char buf[32];
    fd_set rfds, rs;
    struct timeval tv;

    FD_ZERO(&rs);
    FD_SET(0, &rs);
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    tv.tv_sec = 0;
    tv.tv_usec = 200 * 1000;

    int i = 0;
    int r = 0;

    m_bGameStart = false;
    while(1)
    {
        r = select(0 + 1, &rfds, NULL, NULL, &tv);
        if(r < 0)
        {
            write(1,"select() error.\n",16);
            break;
        }
        else if(r == 1)
        {
            read(0, buf + i, 1);
            ++ i;
            if(i > 31)
            {
                write(1,"Too many data\n",14);
                break;
            }
            continue;
        }

        rfds = rs; //恢复rfds，即清除就绪的标准输入文件描述符
        if(i == 2)
        {
            if( buf[0] == '0' )
            {
                return -1;
            }
        }

        i = 0;

        //检查游戏开始标志
        if(m_bGameStart)
        {
            return 0;
        }
    }

    return -1;
}

void CClientMng::set_start(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::Reader reader;
    if(reader.parse(pTask->strMsg, root))
    {
        int nRandomSeed = root["rand_seed"].asInt();
        Json::Value game_players = root["cids"];

        //添加客户端对应的蛇对象
        for(auto iter = game_players.begin();
                iter != game_players.end();
                ++ iter)
        {
            m_pGameClient->add_snake((*iter).asInt());
        }

        //设置随机种子
        m_pGameClient->set_random_seed(root["rand_seed"].asInt());

        //游戏开始标志置位
        {
            std::lock_guard<std::mutex>  lck(m_mtx);
            m_bGameStart = true;
        }
        
        for(int i = 0; i < 3; ++ i)
        {
            printf("\x1b[H\x1b[2J");
            printf("游戏即将开始: %ds\r\n", 3 - i);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    //通知等待游戏开始的客户端
    m_cv.notify_one();
}

void CClientMng::send_game_player_cmd(std::shared_ptr<TTaskData>& pTask)
{
    Json::Value root;
    Json::FastWriter fwriter;
    root["gid"] = pTask->unGameID;
    root["cid"] = pTask->nClientID;
    root["cmd"] = pTask->nOptType;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    0,
                                    m_nServerSockfd,
                                    GAME_PLAYER_CMD,
                                    fwriter.write(root)));
}

int CClientMng::request_start_game(uint64_t gid)
{
    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["gid"] = gid;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    REQ_GAME_START,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "请求开始游戏超时！" << endl;
        enter_any_key_to_continue();
        return -1;
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "请求开始游戏失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }
        else
        {
            //发送开始游戏命令
            root.clear();
            root["gid"] = gid;
            m_queSendMsg.AddTask(make_shared<TTaskData>(
                                        0,
                                        m_nServerSockfd,
                                        GAME_START,
                                        fwriter.write(root)));
        }

        return nRet;
    }
    return -1;
}

void CClientMng::game_start(uint64_t gid)
{
    std::unique_lock<std::mutex> lck(m_mtx);
    m_cv.wait(lck);
    cout << "进入游戏成功!" << endl;

    //进入游戏主循环
    m_pGameClient->play(gid, m_nClientID, 10010);

    //退出游戏
    quit_game_ready(gid);
    quit_game(gid);

    //移除客户端对应的蛇对象
    m_pGameClient->clear_snake();
}

void CClientMng::quit_game(uint64_t gid)
{
    uint64_t nSid = m_cSnowFlake.get_sid();
    Json::Value root;
    Json::FastWriter fwriter;
    root["gid"] = gid;
    root["cid"] = m_nClientID;
    m_queSendMsg.AddTask(make_shared<TTaskData>(
                                    nSid,
                                    m_nServerSockfd,
                                    QUIT_GAME,
                                    fwriter.write(root)));
    std::string result;
    int nRet = m_cEventNotice.WaitNoticeFor(nSid, result, 10);
    if(nRet < 0)
    {
        cout << "退出游戏超时！" << endl;
        enter_any_key_to_continue();
    }

    Json::Reader reader;
    root.clear();
    if(reader.parse(result, root))
    {
        nRet = root["res"].asInt();
        if(nRet < 0)
        {
            cout << "退出游戏失败：" << root["msg"].asString() << endl;
            enter_any_key_to_continue();
        }
    }
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
    thread socketRecvThread([this]()
        {
            this->socket_recv_thread_func();
        });

    thread socketSendThread([this]()
        {
            this->socket_send_thread_func();
        });

    thread taskProcThread([this]()
        {
            this->task_proc_thread_func();
        });

    socketRecvThread.detach();
    socketSendThread.detach();
    taskProcThread.detach();
    return 0;
}

int main()
{
	string IP("192.168.1.188");
	int port = 10086;
	//std::cout<< "请输入ip: ";
	//cin>>IP;
	//std::cout<< "请输入port: ";
	//cin>>port;
	//connect_server(IP.c_str(), port);

    CClientMng clientMng;

    clientMng.init(IP, port);

    clientMng.start_menu();


	return 0;
}
