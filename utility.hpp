#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <algorithm>

#define CLOSE_ATTR  "\033[m"
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define PURPLE      "\033[35m"
#define DEEP_GREEN  "\033[36m"
#define WHITE       "\033[37m"

//计算结构体成员偏移量
#define OFFSET_OF(type, member) ((size_t)&(((type*)0)->member))

/*
** _INTSIZEOF(n)整个做的事情就是将n的长度化为int长度的整数倍。
** 比如n为5，二进制就是101b，int长度为4，二进制为100b，那么n化为int长度的整数倍就应该为8。
** ~(sizeof(int) – 1) )就应该为~（4-1）=~（00000011b）=11111100b，这样任何数& ~(sizeof(int) – 1) )后最后两位肯定为0，就肯定是4的整数倍了。
*/
#define INTSIZE_OF(n) ((sizeof(n) + sizeof(int) – 1) & ~(sizeof(int) – 1))

/*
** 已知结构体type的成员member的地址ptr，求解结构体type的起始地址。
*/
#define CONTAINER_OF(type, member, ptr) ((type*)((char*)ptr - OFFSET_OF(type, member)))

struct TTaskData;
struct TSocketFD;
struct TEventResult;
template<typename T> class CTaskQueue;
template<typename T> class CEventNotice;

typedef CTaskQueue<TSocketFD> 					SOCKETFD_QUE;
typedef CTaskQueue<std::shared_ptr<TTaskData>> 	TASK_QUE;
typedef CEventNotice<std::string>				EVENT_NOTICE;

enum MsgType
{
	REGIST,
	REGIST_RSP,
	LOGIN,
	LOGIN_RSP,
	LOGOUT,
	LOGOUT_RSP,
	HEARTBEAT,
	CHAT,
	QUERY_ROOM,
	QUERY_ROOM_RSP,
	QUERY_ROOM_PLAYERS,
	QUERY_ROOM_PLAYERS_RSP,
	CREATE_ROOM,
	CREATE_ROOM_RSP,
	JOIN_ROOM,
	JOIN_ROOM_RSP,
	QUIT_ROOM,
	QUIT_ROOM_RSP,
	GAME_READY,
	GAME_READY_RSP,
	QUIT_GAME_READY,
	QUIT_GAME_READY_RSP,
	REQ_GAME_START,
	GAME_START,
	QUIT_GAME,
	GAME_PLAYER_CMD
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
	int WaitNoticeFor(uint64_t taskid, T& result, int timeout);

	void Notice(uint64_t taskid, T& result);
private:
	std::map<uint64_t, std::promise<T>> 	m_mapEventResult;
	std::mutex 								m_mtx;
};

template<typename T>
int CEventNotice<T>::WaitNoticeFor(uint64_t taskid, T& result, int timeout)
{
	std::future<T> f;
	{
		std::unique_lock<std::mutex> lck(m_mtx);
		auto iter = m_mapEventResult.find(taskid);
		if(iter == m_mapEventResult.end())
		{
			auto res_pair = m_mapEventResult.insert(
				std::make_pair(taskid, std::promise<T>()));
			f = res_pair.first->second.get_future();
		}
	}

	if(f.valid())
	{
		auto status = f.wait_for(std::chrono::milliseconds(timeout));
		if(status == std::future_status::timeout)
		{
			m_mapEventResult.erase(m_mapEventResult.find(taskid));
			return -1;
		}
		
		m_mapEventResult.erase(m_mapEventResult.find(taskid));
		result = f.get();
		return 0;
	}
	else
	{
		return -2;
	}
}

template<typename T>
void CEventNotice<T>::Notice(uint64_t taskid, T& result)
{
	std::unique_lock<std::mutex> lck(m_mtx);
	auto iter = m_mapEventResult.find(taskid);
	if(iter != m_mapEventResult.end())
	{
		std::promise<T>& ref_p = m_mapEventResult[taskid];
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

	void clear();

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

template<typename T>
void CTaskQueue<T>::clear()
{
	std::lock_guard<std::mutex> lck(m_mtx);
	while(!m_qTask.empty())
	{
		m_qTask.pop();
	}
}

struct TTaskData
{
	TTaskData(){}
	TTaskData(uint64_t id, int fd, MsgType type, std::string msg):
		nTaskId(id), nSockfd(fd), msgType(type), strMsg(msg){}

	TTaskData(MsgType type, uint64_t gid, int cid, int optType):
		msgType(type), unGameID(gid), nClientID(cid), nOptType(optType){}

	uint64_t		nTaskId;	//任务id，唯一表示本次任务
	int 			nSockfd;	//sockfd
	MsgType 		msgType;	//消息类型

	//通用任务数据
	std::string 	strMsg;		//消息内容

	//玩家游戏命令任务数据
	uint64_t      	unGameID;
    int         	nClientID;
    int         	nOptType;
};

struct TMsgHead
{
	TMsgHead(){}
	TMsgHead(MsgType type, size_t len):
		msgType(type), szMsgLength(len){}

	uint64_t		nMsgId;			//消息id
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

class CSnowFlake
{
	/*
	*	bit: 	63 		62~22		21~12	11~0
	*	desc:	默认为0	毫秒时间戳	机器号	序列号
	*/
public:
	CSnowFlake();
	CSnowFlake(uint64_t machineid);
	~CSnowFlake();

	uint64_t get_sid();

private:

	uint64_t 	get_ms_ts();

	uint64_t 	m_nMachineId;

	uint64_t    m_nMaxSerialNum;

	uint64_t	m_nLast_ms_ts;

	uint64_t	m_nKey;

	std::mutex	m_mtx;
};

void setnonblocking(int sock);

void setreuseaddr(int sock);

int readn(int fd, void *vptr, int n);

int writen(int fd, const void *vptr, int n);

void enter_any_key_to_continue();

int get_input_number();

std::string get_input_string();








