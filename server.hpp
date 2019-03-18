#pragma once
#include "utility.hpp"
typedef CTaskQueue<TSocketFD> 				SOCKETFD_QUE;
typedef CTaskQueue<std::shared_ptr<TTaskData>> TASK_QUE;

int create_server(int port);

class CListenThrFunc
{
public:
	CListenThrFunc(SOCKETFD_QUE* pQueSockFD);
	~CListenThrFunc();

	void operator()(int port);

private:
	SOCKETFD_QUE*	m_pQueSockFD;
};

class CSocketRecv
{
public:
	CSocketRecv(SOCKETFD_QUE* pQueSockFD, TASK_QUE* pTaskData);
	~CSocketRecv();

	void operator()();

private:
    SOCKETFD_QUE*	m_pQueSockFD;

    TASK_QUE* 		m_pQueTaskData;
};

class CSocketSend
{
public:
	CSocketSend(TASK_QUE* p);
	~CSocketSend();

	void operator()();

private:
    TASK_QUE* 	m_pQueSendMsg;
};

class CTaskProc
{
public:
    CTaskProc(TASK_QUE* pQueSend, TASK_QUE* pQueTask);
    ~CTaskProc();

    void operator()();

private:
    TASK_QUE* 	m_pQueSendMsg;

    TASK_QUE* 	m_pQueTaskData;
};

class CServerMng
{
public:
	CServerMng();
	~CServerMng();

public:
	void start(int port);

	void init();

	void send_muticast();

	void send_broadcast();

private:
	void init_thread();

private:
	CSocketRecv 	m_socketRecv;

	CSocketSend		m_socketSend;

	CTaskProc		m_taskProc;

	CListenThrFunc 	m_listenThrFunc;

private:
	int 			m_nPort;

	SOCKETFD_QUE 	m_queSockFD;

	TASK_QUE		m_queSendMsg;

	TASK_QUE 		m_queTaskData;
};
