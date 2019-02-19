#pragma once
#include "utility.hpp"

int connect_server(char* server_ip, int server_port);

typedef CTaskQueue<std::shared_ptr<TTaskData>> TASK_QUE;
typedef CTaskQueue<std::shared_ptr<TSocketFD>> SOCKETFD_QUE;

class CSocketRecv
{
public:
	CSocketRecv(int* pKq, int* pFd);
	~CSocketRecv();

	void operator()();

private:
    int* 	m_pKq;
    int*    m_pServerSockfd;
};

class CSocketSend
{
public:
	CSocketSend(TASK_QUE* p, int* pKq);
	~CSocketSend();

	void operator()();

private:
    TASK_QUE* m_pQueSendMsg;
    int* m_pKq;
};

class CTaskProc
{
public:
    CTaskProc(TASK_QUE* pQueSend, TASK_QUE* pQueTask);
    ~CTaskProc();

    void operator()();

private:
    TASK_QUE* m_pQueSendMsg;
    TASK_QUE* m_pQueTaskData;
};

class CClientMng
{
public:
	CClientMng();
	~CClientMng();

public:
	void system_start();

	int regist();

	int login();

	int logout();

	int init(std::string ip, int port);

private:
	

    int connect_server(std::string ip, int port);

    int init_thread();

private:
	CSocketRecv 	m_socketRecv;

	CSocketSend		m_socketSend;

	CTaskProc		m_taskProc;

private:
	SOCKETFD_QUE 	m_queSocketFD;

	TASK_QUE 		m_queTaskData;

	TASK_QUE 		m_queSendMsg;

    int             m_nKq;

    int             m_nServerSockfd;
};
