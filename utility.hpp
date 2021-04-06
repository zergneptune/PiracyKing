#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <functional>
#include <type_traits>

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
       GAME_START
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

struct TTaskData
{
	TTaskData(){}
	TTaskData(uint64_t id, int fd, MsgType type, std::string msg):
		nTaskId(id), nSockfd(fd), msgType(type), strMsg(msg){}

	uint64_t		nTaskId;	//任务id，唯一表示本次任务
	int 			nSockfd;	//sockfd
	MsgType 		msgType;	//消息类型
	std::string 	strMsg;		//消息内容
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


/*
-----------------------------------------
** 常用并行算法 
----------------------------------------
*/

class join_threads
{
public:
    explicit join_threads(std::vector<std::thread>& threads_): m_threads(threads_){}

    ~join_threads()
    {
        for (unsigned long i = 0; i < m_threads.size(); ++i)
        {   
            if (m_threads[i].joinable())
            {
                m_threads[i].join();
            }
        }
    }

private:
    std::vector<std::thread>& m_threads;
};

/*
** std::accumulate 并行版本
*/
template<typename Iterator, typename T>
T accumulate_block(Iterator first, Iterator last)
{
    return std::accumulate(first, last, T()); 
}

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    unsigned long const length = std::distance(first, last);
    if (!length)
        return init;

    unsigned long const min_per_thread = 100000;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;

    unsigned long const hardware_threads = std::thread::hardware_concurrency();

    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

    unsigned long const block_size = length / num_threads;

    std::vector<std::future<T>> futures(num_threads - 1);
    std::vector<std::thread> threads(num_threads - 1);
    join_threads joiner(threads);

    Iterator block_start = first;

    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        std::packaged_task<T(Iterator, Iterator)> task(accumulate_block<Iterator, T>);
        futures[i] = task.get_future();

        threads[i] = std::thread(std::move(task), block_start, block_end);
        block_start = block_end;
    }
    T last_result = accumulate_block<Iterator, T>(block_start, last);
    T result = init;
    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        result += futures[i].get();
    }
    result += last_result;
    return result;
}

//递归划分数据
template<typename Iterator, typename T, typename Func>
T parallel_accumulate_r(Iterator first, Iterator last, T init, Func func)
{
    unsigned long const length = std::distance(first, last);
    if (!length)
        return init;

    unsigned long const max_chunk_size = 100000;
    if (length <= max_chunk_size)
    {
        return std::accumulate(first, last, init, func());
    }
    else
    {
        Iterator mid_point = first + length / 2;
        std::future<T> first_half_result = std::async(parallel_accumulate_r<Iterator, T>, first, mid_point, init);

        T second_half_result = parallel_accumulate_r(mid_point, last, T());
        return first_half_result.get() + second_half_result;
    }
}

/*
 ************************************** 
 * std::for_each 并行版本
 ************************************** 
*/
template<typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f)
{
    unsigned long const length = std::distance(first, last);
    if (!length)
        return;

    unsigned long const min_per_thread = 100000;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;

    unsigned long const hardware_threads = std::thread::hardware_concurrency();

    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

    unsigned long const block_size = length / num_threads;

    std::vector<std::future<void>> futures(num_threads - 1);
    std::vector<std::thread> threads(num_threads - 1);
    join_threads joiner(threads);

    Iterator block_start = first;

    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        std::packaged_task<void(void)> task(
            [=]()
            {
                std::for_each(block_start, block_end, f);
            });
        futures[i] = task.get_future();
        threads[i] = std::thread(std::move(task));
        block_start = block_end;
    }

    std::for_each(block_start, last, f);
    for(unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        futures[i].get();
    }
}

//递归划分数据
template<typename Iterator, typename Func>
void parallel_for_each_r(Iterator first, Iterator last, Func f)
{
    unsigned long const length = std::distance(first, last);
    if (!length)
        return;

    unsigned long const max_chunk_size = 100000;
    if (length <= max_chunk_size)
    {
        std::for_each(first, last, f);
    }
    else
    {
        Iterator const mid_point = first + length / 2;
        std::future<void> first_half = std::async(parallel_for_each_r<Iterator, Func>, first, mid_point, f);
        parallel_for_each_r(mid_point, last, f);
        first_half.get();
    }
}


/*
 * ******************************************************
 * 使用细粒度锁的线程安全队列
 * 1. 队列使用两个互斥元，用来保护head和tail
 * 2. 队列预先分配一个不存储数据的傀儡节点，以保证队列中至少有一个节点, 目的
      是使头尾两个节点的访问分开，以便不需要同时锁住两个互斥元。
 * ******************************************************
*/
template<typename T>
class threadsafe_queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;

public:
    threadsafe_queue() :
        head(new node), tail(head.get())
    {}

    threadsafe_queue(const threadsafe_queue& other) = delete;
    threadsafe_queue& operator=(const threadsafe_queue& other) = delete;

    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool try_pop(T& value)
    {
        std::unique_ptr<node> const old_head = try_pop_head(value);
        return old_head ? true : false;
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_ptr<node> const old_head = wait_pop_head();
        return old_head->data;
    }

    void wait_and_pop(T& value)
    {
        std::unique_ptr<node> const old_head = wait_pop_head(value);
    }

    void push(T new_value)
    {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        //每次从尾巴push一个元素，同时新建一个空元素做新的尾巴
        std::unique_ptr<node> p(new node);
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;
            node* const new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
        }
        data_cond.notify_one();
    }

    bool empty()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get() == get_tail());
    }

private:
    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }
    
    std::unique_ptr<node> pop_head()
    {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    std::unique_lock<std::mutex> wait_for_data()
    {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock, [&](){ return head.get() != get_tail(); });
        return std::move(head_lock);
    }

    std::unique_ptr<node> wait_pop_head()
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head();
    }

    std::unique_ptr<node> wait_pop_head(T& value)
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        value = std::move(*head->data);
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T& value)
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        value = std::move(*head->data);
        return pop_head();
    }
};

/**
 * ****************************
 * 有等待任务的线程池
 * ****************************
 */
class function_wrapper
{
private:
	struct impl_base
	{
		virtual void call() = 0;
		virtual ~impl_base() {}
	};

	std::unique_ptr<impl_base> impl;

	template<typename F>
	struct impl_type : impl_base
	{
		F f;
		impl_type(F&& f_) : f(std::move(f_)) {};
		void call() { f(); }
	};

public:
	template<typename F>
	function_wrapper(F&& f) :
		impl(new impl_type<F>(std::move(f)))
	{}

	void operator() () { impl->call(); }

	function_wrapper() = default;

	function_wrapper(function_wrapper&& other):
		impl(std::move(other.impl))
	{}

	function_wrapper& operator=(function_wrapper&& other)
	{
		impl = std::move(other.impl);
		return *this;
	}

	function_wrapper(const function_wrapper&) = delete;
	function_wrapper(function_wrapper&) = delete;
	function_wrapper& operator=(function_wrapper&) = delete;
};

class thread_pool
{
private:
	std::atomic_bool done;
	threadsafe_queue<function_wrapper> work_queue;
	std::vector<std::thread> threads;
	join_threads joiner;

	//线程函数
	void worker_thread()
	{
		while (!done)
		{
			function_wrapper task;
			if (work_queue.try_pop(task))
			{
                printf("dowork\n");
				task();
			}
			else
			{
				std::this_thread::yield();
			}
		}
	}

public:
	thread_pool() :
		done(false), joiner(threads)
	{
		//设置线程数量
		unsigned const thread_count = std::thread::hardware_concurrency();
		try
		{
			for (unsigned i = 0; i < thread_count; ++i)
			{
                printf("add threadpool\n");
				//加入线程池
				threads.push_back(
						std::thread(&thread_pool::worker_thread, this));
			}
		}
		catch (...)
		{
			done = true;
			throw;
		}
	}

	~thread_pool()
	{
		done = true;
	}

	//提交任务到工作队列
	template<typename FunctionType>
	void submit(FunctionType f)
	{
        printf("do submit\n");
		work_queue.push(std::move(f));
	}
	
	//提交任务到工作队列，并等待结果
	template<typename FunctionType>
	std::future<typename std::result_of<FunctionType()>::type>
	submit_wait_result(FunctionType f)
	{
		typedef typename std::result_of<FunctionType()>::type
			result_type;
		std::packaged_task<result_type()> task(std::move(f));
		std::future<result_type> res(task.get_future());
		//调用function_wrapper的右值构造 
		work_queue.push(std::move(task));
		return res;
	}

	//主动执行正在等待的任务
	void run_pending_task()
	{
		function_wrapper task;
		if (work_queue.try_pop(task))
		{
			task();
		}
		else
		{
			std::this_thread::yield();
		}
	}
};

/*
 *****************************************
 * 使用可等待任务线程池的 parallel_accumulate
 *****************************************
 */

template<typename Iterator, typename T>
T parallel_accumulate_using_thread_pool(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last);
	if (!length) return init;
	unsigned long const block_size = 25;
	unsigned long const num_blocks = (length + block_size - 1) / block_size;

    std::vector<std::future<T>> futures(num_blocks - 1);
	thread_pool pool;

	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_blocks - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		futures[i] = pool.submit_wait_result([block_start, block_end]() -> T {
					return std::accumulate(block_start, block_end, T());
				});
		block_start = block_end;
	}

	T last_result = std::accumulate(block_start, last, T()); 
	T result = init;
	for (unsigned long i = 0; i < (num_blocks - 1); ++i)
	{
		result += futures[i].get();
	}

	result += last_result;
	return result;
}

/*
 *****************************************
 * 基于线程池的快速排序
 *****************************************
 */
template<typename T>
struct sorter
{
	thread_pool pool;

	std::list<T> do_sort(std::list<T>& chunk_data)
	{
		if (chunk_data.empty())
		{
			return chunk_data;
		}

		std::list<T> result;
		result.splice(result.begin(), chunk_data, chunk_data.begin());
		T const& partition_val = *result.begin();

		typename std::list<T>::iterator divide_point =
			std::partition(chunk_data.begin(), chunk_data.end(),
							[&partition_val] (T const& val) { return val < partition_val; });

		std::list<T> new_lower_chunk;
		new_lower_chunk.splice(new_lower_chunk.end(),
								chunk_data, chunk_data.begin(),
								divide_point);

		std::future<std::list<T>> new_lower =
			pool.submit_wait_result(std::bind(&sorter::do_sort, this, std::move(new_lower_chunk)));
			//pool.submit([this, new_lower_chunk(new_lower_chunk)] () { this->do_sort(new_lower_chunk); }); //-std=c++14

		std::list<T> new_higher(do_sort(chunk_data));

		result.splice(result.end(), new_higher);
		while (new_lower.wait_for(std::chrono::seconds(0)) == std::future_status::timeout)
		{
			pool.run_pending_task();
		}

		result.splice(result.begin(), new_lower.get());
		return result;
	}
};

template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
	if (input.empty())
	{
		return input;
	}
	sorter<T> s;
	return s.do_sort(input);
}

//字符串转数字
template<typename T>
class StrConvert
{
public:
    T convert(const std::string& str) {
        std::istringstream istr(str);
        T buf;
        istr >> buf;
        return buf;
    }   
};

//编码转换
void TransCoding(const char* from_code, const char* to_code, const std::string& in, std::string& out);

//分隔字符串
void SplitStr(const std::string& source, const std::string& delimiter, std::vector<std::string>& result);

//连接字符串
std::string joinstr(const std::vector<std::string>& src, const std::string& conn_str);

//字符串转大写
std::string to_upper(const std::string& src);
//字符串转小写
std::string to_lower(const std::string& src);

//检测以指定字符开头的字符串
bool start_with(const std::string& src, const std::string& start);
//检测以指定字符结尾的字符串
bool end_with(const std::string& src, const std::string& end);

//目录遍历
void traverse_dir(const std::string& str_dir, std::vector<std::string>* vec_files, std::vector<std::string>* vec_dirs);

//计算语句调用花费的时间宏
#define CalcTimeFuncInvoke(invoke, desc) {\
    auto start = std::chrono::steady_clock::now();\
    invoke;\
    auto end = std::chrono::steady_clock::now();\
    auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);\
	std::cout << desc << " cost " << time_span.count() << "s" << std::endl;\
}

