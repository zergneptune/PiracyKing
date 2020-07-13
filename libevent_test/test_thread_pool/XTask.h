#pragma once

class XTask
{
public:
    XTask() {}
    ~XTask() {}

public:
    virtual bool Init() = 0;

public:
    struct event_base* m_base = 0;

    int m_n_sock = 0;

    int m_n_thread_id = 0;


private:
    

};

