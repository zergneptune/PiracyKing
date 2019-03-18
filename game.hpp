#pragma once
#include "utility.hpp"

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

class CMap
{
public:
    CMap();
    ~CMap();

    void init();

    void refresh(std::string snake_color);

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
    CFood(CMap& map): m_map(map), m_strColor(GREEN);
    ~CFood();

    void random_make();

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
    CSnake(CMap& map, CFood& food);
    ~CSnake();

public:

    void init();

    void move_up();

    void move_down();

    void move_left();

    void move_right();

    void move_forward();

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

private:
    std::list<CSnakeNode> m_snake;

    CMap&           m_map;

    CFood&          m_food;

    std::mutex      m_mt;

    std::string     m_strColor;
};

enum class GameOpt
{
    MOVE_FORWARD = 0,
    MOVE_UP = 1,
    MOVE_DOWN = 2,
    MOVE_LEFT = 3,
    MOVE_RIGHT = 4
}

struct TGameFrame
{
    size_t      szFrameID:
    GameOpt     opt[2];
}

class CGame
{
    typedef int G_Client; //客户端socketfd
    typedef std::map<G_Client, CTaskQueue<GameOpt>> G_GameOptMap;

public:
    CGame();
    ~CGame();

    void start(int port);

    void over();

    void add_client(G_Client client);

    void add_gameopt(G_Client client, GameOpt opt);

private:
    void send_frame_thread_func(int port);

private:
    G_GameOptMap    m_mapGameOpt;

    std::mutex      m_mtx;

    bool            m_bExit;
}

class CGameServer
{
    typedef size_t  G_GameID;
    typedef std::map<G_GameID, CGame> G_GameMap;

public:
    CGameServer();
    ~CGameServer();

    bool add_game(G_GameID id, CGame& game);

    bool get_game(G_GameID id, CGame& game);
private:
    G_GameMap   m_mapGame;

    std::mutex  m_mtx;
}

class CGameClient
{
public:
    CGameClient();
    ~CGameClient();

    void init();

    void start();

private:
    void recv_frame_thread_func(int port);

    void refresh_thread_func();

private:
    CMap        m_map;

    CFood       m_food;

    CSnake      m_snake[2];

    TASK_QUE&   m_queSendMsg;
}




