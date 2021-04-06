#include <type_traits>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <vector>
#include <list>
#include <algorithm>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include "practice.hpp"
#include "utility.hpp"
//#include "./rb_tree/os_tree_ini.hpp"
#include "./rb_tree/int_tree_ini.hpp"
/*-----------------------*/
#define CalcTimeFuncInvoke(invoke, desc) {\
    auto start = std::chrono::steady_clock::now();\
    invoke;\
    auto end = std::chrono::steady_clock::now();\
    auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);\
	std::cout << desc << " cost " << time_span.count() << "s" << std::endl;\
}

#define HAS_MEMBER(member)\
template<typename T, typename... Args>struct has_member_##member\
{\
	private:\
		template<typename U> static auto Check(int) -> decltype(std::declval<U>().member(std::declval<Args>()...), std::true_type());\
		template<typename U> static std::false_type Check(...);\
	public:\
		enum{value = std::is_same<decltype(Check<T>(0)), std::true_type>::value};\
};


template<typename T, typename... Args>
struct has_member_foo11
{
private:
	template<typename U>
	static auto check(int) ->
	decltype(std::declval<U>().foo(std::declval<Args>()...), std::true_type());

	template<typename U>
	static std::false_type check(...);

public:
	enum{
		value = std::is_same<decltype(check<T>(0)), std::true_type>::value
	};
};

struct MyStruct
{
    string foo() { return ""; }
	int foo(int i) { return i; }
};

template <unsigned n>
struct factorial : std::integral_constant<int,n * factorial<n-1>::value> {};

template <>
struct factorial<0> : std::integral_constant<int,1> {};


typedef int integer_type;
struct A { int x,y; };
struct B { int x,y; };
typedef A C;

////////////////////////////////////////////////////////////////////
// 隐藏光标
#define HIDE_CURSOR() printf("\033[?25l")
// 显示光标
#define SHOW_CURSOR() printf("\033[?25h")

#define CLOSE_ATTR  "\033[m"
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define PURPLE      "\033[35m"
#define DEEP_GREEN  "\033[36m"
#define WHITE       "\033[37m"

#define MAP_W 64
#define MAP_H 32

enum MapType
{
    BLANK = 0,
    BORDER = 1,
    SNAKE = 2,
    FOOD = 3
};

enum CommandType
{
    UNKNOWN = 0,
    MOVE_FORWARD = 1,
    MOVE_UP = 2,
    MOVE_DOWN = 3,
    MOVE_LEFT = 4,
    MOVE_RIGHT = 5
};

class CMap
{
public:
    CMap(){}
    ~CMap(){}

    void init()
    {
        int i, j = 0;
        for(i = 0; i < MAP_H; ++i)
        {
            for(j = 0; j < MAP_W; ++j)
            {
                m_map[i][j] = MapType::BLANK; //空白
                m_overlap[i][j] = 0;
            }
        }

        for(i = 0; i < MAP_H; ++i)
        {
            m_map[i][0] = MapType::BORDER;
            m_map[i][MAP_W - 1] = MapType::BORDER;
        }

        for(j = 0; j < MAP_W; ++j)
        {
            m_map[0][j] = MapType::BORDER;
            m_map[MAP_H - 1][j] = MapType::BORDER;
        }
    }

    void refresh(std::string snake_color)
    {
        int value = 0;
        for(int i = 0; i < MAP_H; ++i)
        {
            for(int j = 0; j < MAP_W; ++j)
            {
                value = m_map[i][j];
                switch(value)
                {
                    case MapType::BORDER:
                        printf("%s*\033[m", BLUE);
                        break;
                    case MapType::SNAKE:
                        printf("%s@\033[m", snake_color.c_str());
                        break;
                    case MapType::FOOD:
                        printf("%s$\033[m", GREEN);
                        break;
                    default:
                        printf(" ");
                }
            }
            printf("\n");
        }
    }

    int* operator[](int i)
    {
        return m_map[i];
    }

    void inc_overlap(int x, int y)
    {
        ++ m_overlap[x][y];
    }

    void dec_overlap(int x, int y)
    {
        -- m_overlap[x][y];
    }

    int get_overlap(int x, int y)
    {
        return m_overlap[x][y];
    }

private:
    int             m_map[MAP_H][MAP_W];

    std::string     m_strColor;

    int             m_overlap[MAP_H][MAP_W];
};

class CFood
{
public:
    CFood(CMap& map): m_map(map), m_strColor(GREEN){}
    ~CFood(){}

    void random_make()
    {
        srand(time(NULL));
        int cox, coy = 0;
        while(1)
        {
            cox = 1 + rand() % (MAP_H - 2);
            coy = 1 + rand() % (MAP_W - 2);
            if(m_map[cox][coy] != MapType::SNAKE)//不为蛇身
            {
                break;
            }
        }
        m_map[cox][coy] = MapType::FOOD;//食物
    }

    void make(int x, int y)
    {
        m_cox = x;
        m_coy = y;
    }

    void set_color(std::string color)
    {
        m_strColor = color;
    }

    std::string get_color()
    {
        return m_strColor;
    }

private:
    int             m_cox;

    int             m_coy;

    CMap&           m_map;

    std::string     m_strColor;
};

class CSnakeNode
{
public:
    CSnakeNode(){}
    CSnakeNode(int x, int y): m_cox(x), m_coy(y){}
    ~CSnakeNode(){}

    int m_cox;

    int m_coy;
};

class CSnake
{
public:
    CSnake(CMap& map, CFood& food): m_map(map), m_food(food), m_strColor(RED){}

    ~CSnake(){}

public:

    void init()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        srand(time(NULL));
        int cox, coy, value = 0;
        while(1)
        {
            cox = 1 + rand() % (MAP_H - 2);
            coy = 1 + rand() % (MAP_W - 2);
            value = m_map[cox][coy];
            if(value != MapType::SNAKE && value != MapType::FOOD)
            {
                if(m_map[cox-1][coy] != MapType::SNAKE && m_map[cox-1][coy] != MapType::FOOD)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox-1][coy] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox-1, coy);
                    m_snake.push_back(CSnakeNode(cox, coy));
                    m_snake.push_back(CSnakeNode(cox-1, coy));
                    break;
                }
                else if(m_map[cox+1][coy] != MapType::SNAKE && m_map[cox+1][coy] != MapType::FOOD)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox+1][coy] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox+1, coy);
                    m_snake.push_back(CSnakeNode(cox, coy));
                    m_snake.push_back(CSnakeNode(cox+1, coy));
                    break;
                }
                else if(m_map[cox][coy-1] != MapType::SNAKE && m_map[cox][coy-1] != MapType::FOOD)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox][coy-1] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox, coy-1);
                    m_snake.push_back(CSnakeNode(cox, coy));
                    m_snake.push_back(CSnakeNode(cox, coy-1));
                    break;
                }
                else if(m_map[cox][coy+1] != MapType::SNAKE && m_map[cox][coy+1] != MapType::FOOD)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox][coy+1] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox, coy+1);
                    m_snake.push_back(CSnakeNode(cox, coy));
                    m_snake.push_back(CSnakeNode(cox, coy+1));
                    break;
                }
            }
        }
    }

    void move_up()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int x1 = iter->m_cox;
        ++ iter;
        int x2 = iter->m_cox;
        if(x1 > x2) //如果蛇正在向下移动，那么禁止向上移动
        {
            return;
        }
        move_core(-1, 0);
    }

    void move_down()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int x1 = iter->m_cox;
        ++ iter;
        int x2 = iter->m_cox;
        if(x1 < x2) //如果蛇正在向上移动，那么禁止向下移动
        {
            return;
        }
        move_core(1, 0);
    }

    void move_left()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int y1 = iter->m_coy;
        ++ iter;
        int y2 = iter->m_coy;
        if(y1 > y2) //如果蛇正在向右移动，那么禁止向左移动
        {
            return;
        }
        move_core(0, -1);
    }

    void move_right()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int y1 = iter->m_coy;
        ++ iter;
        int y2 = iter->m_coy;
        if(y1 < y2) //如果蛇正在向左移动，那么禁止向右移动
        {
            return;
        }
        move_core(0, 1);
    }

    void move_forward()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int x1 = iter->m_cox;
        int y1 = iter->m_coy;
        ++ iter;
        int x2 = iter->m_cox;
        int y2 = iter->m_coy;
        if(x1 > x2)//向下移动
        {
            move_core(1, 0);         
        }
        else if(x1 < x2)//向上移动
        {
            move_core(-1, 0);
        }
        else if(y1 > y2)//向右移动
        {
            move_core(0, 1);
        }
        else
        {
            move_core(0, -1);
        }
    }

    void set_color(std::string color)
    {
        m_strColor = color;
    }

    std::string get_color()
    {
        return m_strColor;
    }

private:
    void move_core(int r_x, int r_y) //参数为相对移动距离
    {
        auto iter = m_snake.begin();
        int move_to_x = iter->m_cox + r_x;
        int move_to_y = iter->m_coy + r_y;
        iter = m_snake.end();
        -- iter;
        int last_x = iter->m_cox;
        int last_y = iter->m_coy;
        if(m_map[move_to_x][move_to_y] == MapType::BORDER)
        {
            return;
        }
        else if(m_map[move_to_x][move_to_y] == MapType::FOOD)
        {
            m_snake.push_front(CSnakeNode(move_to_x, move_to_y));
            m_map[move_to_x][move_to_y] = MapType::SNAKE;
            m_map.inc_overlap(move_to_x, move_to_y);
            m_food.random_make();
            return;
        }

        m_map.inc_overlap(move_to_x, move_to_y);
        m_map[move_to_x][move_to_y] = MapType::SNAKE;
        m_map.dec_overlap(last_x, last_y);//蛇尾离开，减少蛇尾位置重叠数
        if(m_map.get_overlap(last_x, last_y) < 1)//如果蛇尾位置没有重叠
        {
            m_map[last_x][last_y] = MapType::BLANK;
        }
        iter->m_cox = move_to_x;
        iter->m_coy = move_to_y;
        m_snake.splice(m_snake.begin(), m_snake, iter);
    }

private:
    std::list<CSnakeNode> m_snake;

    CMap&           m_map;

    CFood&          m_food;

    std::mutex      m_mt;

    std::string     m_strColor;
};

void thread_func(CSnowFlake* p)
{
    std::thread::id id = std::this_thread::get_id();
    uint64_t sid = 0;
    for(int i = 0; i < 10; ++ i)
    {
        sid = p->get_sid();
        std::cout << " thread " << id << " get: " << sid << '\n';
    }
}

class TA
{
public:
    void operator()() {
        printf("hello world!\n");
    }
};

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;
    /*
    CGame game;
    game.init();
    game.start();
    */
    /*CSnowFlake  snowflake(0);
    std::vector<std::thread> v;
    for(int i = 0; i < 4; ++ i)
    {
        v.push_back(std::thread(thread_func, &snowflake));
    }

    std::for_each(v.begin(), v.end(), [](std::thread& th){
        th.join();
    });*/
    RBTree<int> int_rbt;
    //std::vector<int> vec{0, 4, 2, 5, 1, 3, 4, 8, 2};
    std::vector<int> vec{8, 1, 5, 0, 6, 3, 1, 9, 9};
    for (auto k : vec)
    {
        std::cout << "k = " << k << std::endl;
        int_rbt.insert(k, k, k + 5);
        int_rbt.in_order();
    }

    auto res = int_rbt.interval_search(15, 16);
    std::cout << "search res: " << res << std::endl;
    /*
    for (int i = 0; i < 10; ++i)
    {
        std::cout << "search " << i << ", ";
        auto res = int_rbt.search_k(i);
        if (res)
        {
            std::cout << "OK" << res->data << std::endl;
        }
        else
        {
            std::cout << "FAIL" << std::endl;
        }
    }
    */

    for (auto k : vec)
    {
        std::cout << "k = " << k << std::endl;
        if(int_rbt.remove(k))
        {
            std::cout << "remove [" << k << "] : ";
            int_rbt.in_order();
        }
    }
    std::cout << std::endl;

    thread_pool tp;
    printf("debug\n");
    tp.submit(TA());
    //tp.run_pending_task();
    sleep(5);


    return 0;
}
