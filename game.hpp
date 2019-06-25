#pragma once
#include "utility.hpp"
#include <list>

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

#define MAP_W 50
#define MAP_H 28

enum MapType
{
    BLANK = 0,
    BORDER,
    SNAKE,
    FOOD
};

enum GameOptType
{
    MOVE_FORWARD = 0,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT
};

struct TGameFrame
{
    size_t      szFrameID;

    uint64_t    nGameId;

    int         optType[2];
};

struct TGameFrameUdp
{
    size_t      szFrameID;

    uint64_t    nGameId;

    int         nClientID[2];

    int         optType[2];
};

struct TGameCmd
{
    TGameCmd(){}
    TGameCmd(size_t gid, int cid, int type):
        szGameID(gid), nClientID(cid), optType(type){}

    size_t      szGameID;

    int         nClientID;

    int         optType;
};

class CMap
{
public:
    CMap();
    ~CMap();

    void init();

    void refresh();

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

    void random_make_food();

private:
    std::string     m_strMapColor;

    std::string     m_strFoodColor;

    int             m_map[MAP_H][MAP_W];

    int             m_overlap[MAP_H][MAP_W];
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
    CSnake();
    CSnake(CMap* pmap, std::string color = RED);
    ~CSnake();

public:

    void init();

    void move_up();

    void move_down();

    void move_left();

    void move_right();

    void move_forward();

    GameOptType get_move_direction();

    void set_color(std::string color)
    {
        m_strColor = color;
    }

    std::string get_color()
    {
        return m_strColor;
    }

    void add_node(CSnakeNode node)
    {
        m_snake.push_back(node);
    }

    void vanish();

private:
    void move_core(int r_x, int r_y); //参数为相对移动距离

private:
    std::list<CSnakeNode>   m_snake;

    CMap*           m_pMap;

    std::string     m_strColor;

    std::mutex      m_mt;
};

class CGame
{
    struct TPlayerStatus
    {
        TPlayerStatus(): m_nReadyGame(0){}
        ~TPlayerStatus(){}

        int  m_nReadyGame;
    };

    typedef uint64_t G_GameID;
    typedef int G_ClientID; //客户端
    typedef int G_RoomOwner;//房主
    typedef std::map<G_ClientID, std::shared_ptr<CTaskQueue<int>>> G_GameOptMap;
    typedef std::map<G_ClientID, TPlayerStatus> G_PlayerStatusMap;

public:
    CGame(G_GameID gameID, std::string strName);
    ~CGame();

    void start(int port);

    void over();

    bool add_client(G_ClientID client);

    bool remove_client(G_ClientID client);

    bool ready(G_ClientID client);

    bool quit_ready(G_ClientID client);

    bool get_ready_status(G_ClientID client);

    bool get_running_status();

    bool is_all_ready();

    int get_client_nums();

    void get_client_ids(std::vector<int>& vecCids);

    std::string get_name(){ return m_strRoomName; }

    int set_room_owner();

    int set_room_owner(G_ClientID cid);

    int get_room_owner(){ return m_roomOwner; }

    void add_gameopt(G_ClientID client, GameOptType type);

    void get_game_frame(TGameFrameUdp& temp_frame);

    bool is_game_running(){ return m_bIsGameRunning; }

private:
    void send_frame_thread_func(int port);

private:

    G_GameID                    m_gameID;

    std::string                 m_strRoomName;

    G_GameOptMap                m_mapGameOpt;

    G_PlayerStatusMap           m_mapPlayerStatus;

    std::mutex                  m_mtx;

    std::condition_variable     m_cv;

    bool                        m_bExitSendFrame;

    bool                        m_bIsGameRunning;

    G_RoomOwner                 m_roomOwner;

    size_t                      m_szGameFrameCnt;
};

class CGameServer
{
    typedef uint64_t G_GameID;
    typedef std::map<G_GameID, std::shared_ptr<CGame>> G_GameMap;

public:
    CGameServer();
    ~CGameServer();

    uint64_t create_game(int cid, std::string& strGameName);

    void remove_game(G_GameID gid);

    void remove_player(int cid);

    void remove_player(G_GameID gid, int cid);

    int add_player(G_GameID gid, int cid);

    void add_game_player_opt(G_GameID gid, int cid, GameOptType type);

    int game_ready(G_GameID gid, int cid);

    int quit_game_ready(G_GameID gid, int cid);

    void game_start(G_GameID gid);

    void game_over(G_GameID gid);

    bool get_game_ready_status(G_GameID gid);

    bool get_game_running_status(G_GameID gid);

    std::shared_ptr<CGame> get_game(G_GameID gid);

    std::string get_gameid_list();

    void get_cid_list(G_GameID id, std::vector<int>& vecCids);

    void get_game_list(std::vector<std::shared_ptr<CGame>>& vecGames);

    int get_room_owner(G_GameID id);

    int get_player_nums(G_GameID id);

private:

    bool is_game_name_existed(std::string& strGameName);

    G_GameMap   m_mapGame;

    CSnowFlake  m_cSnowFlake;

    std::mutex  m_mtx;
};

class CGameClient
{
    typedef uint64_t G_GameID;
    typedef int G_ClientID;
    typedef CTaskQueue<std::shared_ptr<TGameFrameUdp>> GameFrameQue;
    typedef CTaskQueue<std::shared_ptr<TGameCmd>>   GameCmdQue;
    typedef std::map<G_ClientID, std::shared_ptr<CSnake>>   SnakeMap;
public:
    CGameClient(TASK_QUE*);
    ~CGameClient();

    void play(G_GameID gid, int cid, int port);

    void set_random_seed(int seed){ m_nRandSeed = seed; }

    void add_snake(G_ClientID);

    void remove_snake(G_ClientID); //移走蛇的同时，清空地图上的位置

    void clear_snake();

    void random_make_snake();

    void add_game_frame(TGameFrameUdp* pGameFrame);

private:
    void init();

    bool is_valid_move(int cid, GameOptType opttype);

    void recv_frame_thread_func(int port);

    void refresh_thread_func();

private:
    CMap            m_map;

    std::mutex      m_mtx_snake;

    SnakeMap        m_mapSnake;

    GameCmdQue      m_QueGameCmd;

    GameFrameQue    m_queGameFrame;

    bool            m_bExitRecvFrame;

    bool            m_bExitRefresh;

    int             m_nRandSeed; //随机种子

private:
    TASK_QUE*       m_pTaskData;
};




