#pragma once
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

#define IF_EXIT(predict, err) if(predict){ perror(err); exit(1); }

void setnonblocking(int sock);

void setreuseaddr(int sock);

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







