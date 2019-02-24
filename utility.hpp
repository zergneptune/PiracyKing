#pragma once
#include <iostream>
#include <string>
#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>

enum MsgType
{
	REGIST = 0,
	REGIST_RSP = 1,
	LOGIN = 2,
	LOGIN_RSP = 3,
	LOGOUT = 4,
	HEARTBEAT = 5,
	CHAT = 6
};

#define IF_EXIT(predict, err) if(predict){ perror(err); exit(1); }

class CSemaphore
{
public:
	CSemaphore();
	~CSemaphore();

public:
	void Wait();
	int WaitFor(int ms);
	void NoticeOne();
	void NoticeAll();

private:
	std::mutex m_mtx;
	
	std::condition_variable m_cv;
};

template<typename T>
class CEventNotice
{
public:
	CEventNotice(){}
	~CEventNotice(){}

public:
	int WaitNoticeFor(std::string& strTaskID, T& result, int timeout);

	void Notice(std::string& strTaskID, T& result);
private:
	std::map<std::string, std::promise<T>> 	m_mapEventResult;
	std::mutex 								m_mtx;
};

template<typename T>
int CEventNotice<T>::WaitNoticeFor(std::string& strTaskID, T& result, int timeout)
{
	std::future<T> f;
	{
		std::unique_lock<std::mutex> lck(m_mtx);
		auto iter = m_mapEventResult.find(strTaskID);
		if(iter == m_mapEventResult.end())
		{
			m_mapEventResult.insert(std::make_pair(strTaskID, std::promise<T>()));
			f = m_mapEventResult[strTaskID].get_future();
		}
	}

	if(f.valid())
	{
		auto status = f.wait_for(std::chrono::milliseconds(timeout));
		if(status == std::future_status::timeout)
		{
			std::cout << "等待事件超时!" << std::endl;
			m_mapEventResult.erase(m_mapEventResult.find(strTaskID));
			return -1;
		}
		
		m_mapEventResult.erase(m_mapEventResult.find(strTaskID));
		result = f.get();
		return 0;
	}
	else
	{
		return -2;
	}
}

template<typename T>
void CEventNotice<T>::Notice(std::string& strTaskID, T& result)
{
	std::unique_lock<std::mutex> lck(m_mtx);
	auto iter = m_mapEventResult.find(strTaskID);
	if(iter != m_mapEventResult.end())
	{
		std::promise<T>& ref_p = m_mapEventResult[strTaskID];
		ref_p.set_value(result);
	}
}

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
	TTaskData(std::string id, int fd, MsgType type, std::string msg):
		strTaskID(id), nSockfd(fd), msgType(type), strMsg(msg){}

	std::string 	strTaskID;	//任务id，唯一标示本次任务
	int 			nSockfd;	//发送目标sockfd
	MsgType 		msgType;	//消息类型
	std::string 	strMsg;		//消息内容
};

struct TMsgHead
{
	TMsgHead(){}
	TMsgHead(MsgType type, size_t len):
		msgType(type), szMsgLength(len){}

	MsgType 		msgType; 		//消息类型
	size_t 			szMsgLength; 	//消息长度
};

struct TSocketFD
{
	TSocketFD(){}
	TSocketFD(int fd): sockfd(fd){}
	~TSocketFD(){}
	
	int sockfd;
};

void setnonblocking(int sock);

void setreuseaddr(int sock);

int readn(int fd, void *vptr, int n);

int writen(int fd, const void *vptr, int n);

int get_input_number();

std::string get_input_string();








