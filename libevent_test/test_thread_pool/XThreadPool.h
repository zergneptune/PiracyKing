#pragma once
#include <vector>
#include "XThread.h"
class XThread;
class XThreadPool;
class XTask;

class XThreadPool
{
public:
    static XThreadPool* Get()
    {
        static XThreadPool p;
        return &p;
    }
    ~XThreadPool() {}

    //初始化所有线程并启动线程
    void Init(int threadCount);

    //分发线程
    void Dispatch(XTask* task);



private:
    XThreadPool() {}

    int m_nThreadCount = 0;

    int m_nLastThread = -1;

    std::vector<XThread*> m_vecThreads;
};

