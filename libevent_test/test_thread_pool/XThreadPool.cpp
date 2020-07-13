#include "XThreadPool.h"
#include "XThread.h"
#include <thread>
#include <iostream>
//初始化并启动线程
void XThreadPool::Init(int threadCount)
{
    this->m_nThreadCount = threadCount;
    m_nLastThread = -1;
    for(int i = 0; i < threadCount; ++i)
    {
        XThread* t = new XThread();
        t->m_nId = i;
        std::cout << "Create thread " << i << std::endl;
        //启动线程
        t->Start();

        m_vecThreads.push_back(t);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//分发线程
void XThreadPool::Dispatch(XTask* task)
{
    //轮询
    if(!task) return;

    int tid = (m_nLastThread + 1) % m_nThreadCount;
    m_nLastThread = tid;
    XThread* t = m_vecThreads[tid];

    //给线程添加任务
    t->AddTask(task);

    //激活线程
    t->Activate();
}

