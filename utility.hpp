#pragma once
#include <iostream>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <string>
using std::cin;
using std::cout;

enum MsgType
{
	REGIST = 0,
	LOGIN = 1,
	LOGOUT = 2,
	HEARTBEAT = 3,
	CHAT = 4
};

#define IF_EXIT(predict, err) if(predict){ perror(err); exit(1); }

template<typename T>
class CTaskQueue
{
public:
	CTaskQueue(){}
	~CTaskQueue(){}

	void AddTask(T pNewTask);

	T Wait_GetTask();

	bool Try_GetTask(T& pTask);

	bool Empty();

private:
	std::queue<T> m_qTask;
	std::mutex m_mtx;
	std::condition_variable m_cv;
};

template<typename T>
void CTaskQueue<T>::AddTask(T pNewTask)
{
	std::lock_guard<std::mutex> lck(m_mtx);
	m_qTask.push(pNewTask);
	m_cv.notify_one();
}

template<typename T>
T CTaskQueue<T>::Wait_GetTask()
{
	std::unique_lock<std::mutex> lck(m_mtx);
	m_cv.wait(lck, [this](){ return !m_qTask.empty(); });
	T res = m_qTask.front();
	m_qTask.pop();
	return res;
}

template<typename T>
bool CTaskQueue<T>::Try_GetTask(T& pTask)
{
	std::lock_guard<std::mutex> lck(m_mtx);
	if(m_qTask.empty())
	{
		return false;
	}
	pTask = m_qTask.front();
	m_qTask.pop();
	return true;
}

template<typename T>
bool CTaskQueue<T>::Empty()
{
	std::lock_guard<std::mutex> lck(m_mtx);
	return m_qTask.empty();
}

struct TTaskData
{
	TTaskData(){}
	TTaskData(int sockfd, MsgType type, std::string msg): nSockfd(sockfd), msgType(type), strMsg(msg){}

	int nSockfd; 		//发送目标sockfd
	MsgType msgType; 	//消息类型
	std::string strMsg; //消息内容
};

struct TMsgHead
{
	TMsgHead(){}
	TMsgHead(MsgType type, size_t len): msgType(type), szMsgLength(len){}

	MsgType msgType; 	//消息类型
	size_t szMsgLength; //消息长度
};

struct TSocketFD
{

};

void setnonblocking(int sock);

void setreuseaddr(int sock);

int readn(int fd, void *vptr, int n);

int writen(int fd, const void *vptr, int n);

int get_input_number();

std::string get_input_string();








