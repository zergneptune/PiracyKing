#include <type_traits>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <vector>
#include <list>
#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <dirent.h>
#include "practice.hpp"
#include "utility.hpp"
#include "poker.hpp"
using namespace NSP_POKER;
/*-------  test ---------*/
#define LUA_ROOT    "/usr/local/"
#define LUA_LDIR    LUA_ROOT "share/lua/5.1/"
#define LUA_CDIR    LUA_ROOT "lib/lua/5.1/"
#define LUA_PATH_DEFAULT  \
        "./?.lua;"  LUA_LDIR"?.lua;"  LUA_LDIR"?/init.lua;" \
                    LUA_CDIR"?.lua;"  LUA_CDIR"?/init.lua"
/*-----------------------*/
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
                        printf("\033[30m*\033[m");
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
    CSnake(CMap& map, CFood& food): m_map(map), m_food(food), m_strColor(RED), m_nSpeed(1){}

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

    bool move_up()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int x1 = iter->m_cox;
        ++ iter;
        int x2 = iter->m_cox;
        if(x1 != x2) //如果蛇正在向下移动，那么禁止向上移动
        {
            return false;
        }
        move_core(-1, 0);
        return true;
    }

    bool move_down()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int x1 = iter->m_cox;
        ++ iter;
        int x2 = iter->m_cox;
        if(x1 != x2) //如果蛇正在向上移动，那么禁止向下移动
        {
            return false;
        }
        move_core(1, 0);
        return true;
    }

    bool move_left()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int y1 = iter->m_coy;
        ++ iter;
        int y2 = iter->m_coy;
        if(y1 != y2) //如果蛇正在向右移动，那么禁止向左移动
        {
            return false;
        }
        move_core(0, -1);
        return true;
    }

    bool move_right()
    {
        std::lock_guard<std::mutex> lock(m_mt);
        auto iter = m_snake.begin();
        int y1 = iter->m_coy;
        ++ iter;
        int y2 = iter->m_coy;
        if(y1 != y2) //如果蛇正在向左移动，那么禁止向右移动
        {
            return false;
        }
        move_core(0, 1);
        return true;
    }

    bool move_forward()
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
        return true;
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
    void incr_speed(){ ++ m_nSpeed; }

    void move_core(int r_x, int r_y) //参数为相对移动距离
    {
        auto iter = m_snake.begin();
        int move_to_x = iter->m_cox + r_x;
        int move_to_y = iter->m_coy + r_y;
        iter = m_snake.end();
        -- iter;
        int last_x = iter->m_cox;
        int last_y = iter->m_coy;
        if(move_to_x < 0 || move_to_y < 0)
        {
            return;
        }

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
            incr_speed();
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

    int             m_nSpeed;
};

class CWorkThreadFunc
{
public:
    CWorkThreadFunc(CTaskQueue<CommandType>& queCmd, CSnake& snake, CMap& map): 
        m_bToExit(false), m_queCmd(queCmd), m_snake(snake), m_map(map){}
    ~CWorkThreadFunc(){}

public:
    void operator()()
    {
        m_bToExit = false;
        CommandType cmd_type = UNKNOWN;
        bool bRefresh = false;
        while(!m_bToExit)
        {
            cmd_type = m_queCmd.Wait_GetTask();
            switch(cmd_type)
            {
                case MOVE_FORWARD:
                    bRefresh = m_snake.move_forward();
                    break;
                case MOVE_UP:
                    bRefresh = m_snake.move_up();
                    break;
                case MOVE_DOWN:
                    bRefresh = m_snake.move_down();
                    break;
                case MOVE_LEFT:
                    bRefresh = m_snake.move_left();
                    break;
                case MOVE_RIGHT:
                    bRefresh = m_snake.move_right();
                    break;
                default:
                    break;
            }
            if(bRefresh)
            {
                printf("\x1b[H\x1b[2J");
                m_map.refresh(m_snake.get_color());
            }
        }
    }

    void set_exit()
    {
        m_bToExit = true;
    }

private:
    bool m_bToExit;

    CTaskQueue<CommandType>&    m_queCmd;

    CSnake&                     m_snake;

    CMap&                       m_map;
};

class CGame
{
public:
    CGame(): m_food(m_map), m_snake(m_map, m_food), 
                m_workFunc(m_queCmd, m_snake, m_map){}
    ~CGame(){}

public:

    void init()
    {
        m_map.init();
        m_snake.init();
        m_food.random_make();
    }

    void start()
    {
        std::thread work_thread(m_workFunc);
        work_thread.detach();
        bool bExitThread = false;
        std::thread auto_move_thread([this, &bExitThread](){
            while(!bExitThread)
            {
                m_queCmd.AddTask(MOVE_FORWARD);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });

        HIDE_CURSOR();
        fd_set rfds, rs;
        struct timeval tv;

        int i,r,q,j,dir;
        struct termios saveterm, nt;
        char c,buf[32],str[8];

        tcgetattr(0, &saveterm);
        nt = saveterm;

        nt.c_lflag &= ~ECHO;
        nt.c_lflag &= ~ISIG;   
        nt.c_lflag &= ~ICANON;

        tcsetattr(0, TCSANOW, &nt);

        FD_ZERO(&rs);
        FD_SET(0, &rs);
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        tv.tv_sec=0;
        tv.tv_usec=0;

        i = 0; q = 0; dir = 0;
        while(1)
        {
            read(0 , buf+i, 1);
            i++;
            if(i>31)
            {
                write(1,"Too many data\n",14);
                break;
            }
            //write(1, str, 4);
            r = select(0 + 1, &rfds, NULL, NULL, &tv); //0：监听标准输入，若r=1，说明标准输入可读，rfds中标准输入文件描述符会就绪
            if(r<0)
            {
                write(1,"select() error.\n",16);
                break;
            }
            if(r == 1)
                continue;
            //write(1, "\t", 1);
            rfds = rs; //恢复rfds，即清除就绪的标准输入文件描述符
            if(i == 3 && buf[0] == 0x1b && buf[1] == 0x5b)
            {
                c = buf[2];
                switch(c)
                {
                    case 0x41:
                        m_queCmd.AddTask(MOVE_UP);
                        break;
                    case 0x42:
                        m_queCmd.AddTask(MOVE_DOWN);
                        break;
                    case 0x43:
                        m_queCmd.AddTask(MOVE_RIGHT);
                        break;
                    case 0x44:
                        m_queCmd.AddTask(MOVE_LEFT);
                        break;
                    default:
                        break;
                }
            }
            //确保两次连续的按下ESC键，才退出
            if(buf[0] == 27 && i == 1)
            {
                if(q == 0)
                    q = 1;
                else
                    break;
            }
            else
                q = 0;
            i = 0;
        }

        tcsetattr(0, TCSANOW, &saveterm);
        SHOW_CURSOR();
        bExitThread = true;//退出蛇自动前进的线程
        auto_move_thread.join();
    }

private:
    CMap    m_map;

    CFood   m_food;

    CSnake  m_snake;

    CWorkThreadFunc         m_workFunc;

    CTaskQueue<CommandType> m_queCmd;
};

int readFileList(const char *basePath, std::vector<std::string>& vec_img_name)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];

    if((dir = opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    while((ptr = readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..")==0)
            continue;
        else if(ptr->d_type == 8)    ///file
        {
            vec_img_name.push_back(string(ptr->d_name));
        }
        else if(ptr->d_type == 10)    ///link file
            printf("d_name: %s/%s\n", basePath, ptr->d_name);
        else if(ptr->d_type == 4)    ///dir
        {
            memset(base, '\0', sizeof(base));
            strcpy(base, basePath);
            strcat(base, "/");
            strcat(base, ptr->d_name);
            readFileList(base, vec_img_name);
        }
    }
    closedir(dir);
    return 1;
}

void picture_tagging()
{
    std:cout << "输入图片文件路径：";
    std::string str_pic_path = get_input_string();
    std::cout << "输入图片信息文件路径：";
    std::string str_pic_info_path = get_input_string();
    size_t sz_start_pos = str_pic_info_path.find_last_of("/\\");
    std::string str_pic_tagging_res_path;
    
    if(sz_start_pos == std::string::npos)
    {
        std::cout << "error file name: " << str_pic_info_path << endl;
        return; 
    }

    std::string str_pic_info_filename;
    if(sz_start_pos != 0)
    {
        str_pic_info_filename = str_pic_info_path.substr(sz_start_pos + 1);
    }
    else
    {
        str_pic_info_filename = str_pic_info_path.substr(1);
    }

    size_t pos = str_pic_info_filename.rfind(".");
    if(pos != std::string::npos)
    {
        str_pic_info_filename = str_pic_info_filename.substr(0, pos);
    }
    std::string str_pic_tagging_res_file_name = str_pic_info_filename + string("_tagging_reuslt.txt");
    str_pic_tagging_res_path = str_pic_info_path.substr(0, sz_start_pos + 1) + str_pic_tagging_res_file_name;
    printf("str_pic_info_filename = %s\n", str_pic_info_filename.c_str());
    printf("str_pic_tagging_res_file_name = %s\n", str_pic_tagging_res_file_name.c_str());
    
    int n_pic_num = 0;
    int n_pic_tag = 0;
    int n_start_line_num = 0;
    int n_readed_line_num = 0;
    std::string str_line_info;
    std::string str_pic_url, str_link, str_text, str_pic_text;
    std::ifstream ifs;
    std::ofstream ofs;

    ifs.open(str_pic_info_path);
    if(!ifs.is_open())
    {
        std::cout << "error open: " << str_pic_info_path << endl;
        return;
    }

    ofs.open(str_pic_tagging_res_path, std::ofstream::out | std::ofstream::app);
    if(!ofs.is_open())
    {
        std::cout << "error open: " << str_pic_tagging_res_path << endl;
        return;
    }

    std::vector<std::string> vec_img_name;
    readFileList(str_pic_path.c_str(), vec_img_name);
    std::sort(vec_img_name.begin(), vec_img_name.end(), std::less<std::string>());
    for(auto& ref : vec_img_name)
    {
        std::cout << ref << endl;
    }

    std::cout << "输入开始标注的图片序号：";
    n_pic_num = get_input_number();
    n_start_line_num = n_pic_num + 1;
    for(int i = 1; i < n_start_line_num; ++i)
    {
        ifs.ignore(10240, '\n');
    }

    char buf[32] = { 0 };
    sprintf(buf, "%d.jpg", n_pic_num);
    std::string start_pic_name(buf);
    for(auto& img_name : vec_img_name)
    {
        if(img_name >= start_pic_name)
        {
            std::string tmp = img_name.substr(0, img_name.find(".") + 1);
            int img_num = atoi(tmp.c_str());
            ifs.seekg(std::ifstream::beg);
            n_start_line_num = img_num + 1;
            for(int i = 1; i < n_start_line_num; ++i)
            {
                ifs.ignore(40960, '\n');
            }
            std::getline(ifs, str_line_info);
            std::stringstream strs(str_line_info);
            strs >> str_pic_url >> str_link >> str_text >> str_pic_text;
            std::cout << "当前标注的图片：" << PURPLE << "[ " << img_name << " ]" << CLOSE_ATTR << endl;
            std::cout << "\t广告图片链接地址：" << str_pic_url << endl;
            std::cout << "\t广告落地页：" << str_link << endl;
            std::cout << "\t广告文字：" << BLUE << str_text << CLOSE_ATTR << endl;
            std::cout << "\t广告图片中文字：" << BLUE << str_pic_text << CLOSE_ATTR << endl;
            std::cout << "\t输入图片标签：";
            n_pic_tag = get_input_number();
            ofs << img_num << '\t' << n_pic_tag << "\r\n";
            ofs.flush();
        }
    }

    ifs.close();
    ofs.close();
}

class CPokerCardMng
{
public:
    CPokerCardMng(){}
    ~CPokerCardMng(){}

public:
    void init()
    {
        for(int i = 1; i < 14; ++ i)
        {
            m_vecPokerCards.push_back(CPokerCard(i, enumPokerColor::SPADE));
        }

        for(int i = 1; i < 14; ++ i)
        {
            m_vecPokerCards.push_back(CPokerCard(i, enumPokerColor::HEART));
        }

        for(int i = 1; i < 14; ++ i)
        {
            m_vecPokerCards.push_back(CPokerCard(i, enumPokerColor::CLUB));
        }

        for(int i = 1; i < 14; ++ i)
        {
            m_vecPokerCards.push_back(CPokerCard(i, enumPokerColor::DIAMOND));
        }

        m_vecPokerCards.push_back(CPokerCard(14));
        m_vecPokerCards.push_back(CPokerCard(15));
        assert(m_vecPokerCards.size() == 54);
    }

    bool random_get_one(CPokerCard& card)
    {
        uint64_t seed = m_snowFlake.get_sid();
        srand(seed);
        int nPokerNum = m_vecPokerCards.size();
        if(nPokerNum == 0)
        {
            return false;
        }
        else
        {
            int nIndex = rand() % nPokerNum;
            card = m_vecPokerCards[nIndex];
            return true;
        }
    }

private:
    std::vector<CPokerCard>     m_vecPokerCards;
    CSnowFlake                  m_snowFlake;
};

void print_poker_card()
{
    CPokerCard pokerCard;
    CPokerCardMng pokerCardMng;
    pokerCardMng.init();
    int nPokerNum = 10;
    for(int i = 0; i < nPokerNum; ++i)
    {
        pokerCardMng.random_get_one(pokerCard);
        int nPokerNumber = pokerCard.get_number();
        enumPokerColor pokerColor = pokerCard.get_color();
        bool bIsJoker = false;
        std::string print_number;
        std::string print_color;
        std::string joker_color;
        switch(nPokerNumber)
        {
            case 1:
                print_number = "Ａ";
                break;
            case 2:
                print_number = "２";
                break;
            case 3:
                print_number = "３";
                break;
            case 4:
                print_number = "４";
                break;
            case 5:
                print_number = "５";
                break;
            case 6:
                print_number = "６";
                break;
            case 7:
                print_number = "７";
                break;
            case 8:
                print_number = "８";
                break;
            case 9:
                print_number = "９";
                break;
            case 10:
                print_number = "10";
                break;
            case 11:
                print_number = "Ｊ";
                break;
            case 12:
                print_number = "Ｑ";
                break;
            case 13:
                print_number = "Ｋ";
                break;
            case 14:
                bIsJoker = true;
                joker_color = BLACK;
                break;
            case 15:
                bIsJoker = true;
                joker_color = RED;
                break;
            default:
                break;
        }

        switch(pokerColor)
        {
            case enumPokerColor::SPADE:
                print_color = "♠️";
                break;
            case enumPokerColor::HEART:
                print_color = "♥️";
                break;
            case enumPokerColor::CLUB:
                print_color = "♣️";
                break;
            case enumPokerColor::DIAMOND:
                print_color = "♦️";
                break;
            default:
                break;
        }


        if(i < nPokerNum - 1)
        {
            if(i == 0)
            {
                printf("  ");
                for(int j = 0; j < 6; ++j)
                {
                    printf("_");
                }
            }
            else
            {
                MOVEUP(11);
                for(int j = 0; j < 7; ++j)
                {
                    printf("_");
                }
            }

            printf("\r\n");
            MOVERIGHT(i * 7);

            if(bIsJoker)
            {
                printf("|%sＪ%s", joker_color.c_str(), CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%sＯ%s", joker_color.c_str(), CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%sＫ%s", joker_color.c_str(), CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%sＥ%s", joker_color.c_str(), CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%sＲ%s", joker_color.c_str(), CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);
            }
            else
            {
                printf("|%s", print_number.c_str());
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%s", print_color.c_str());
                printf("\r\n");
                MOVERIGHT(i * 7);

                for(int j = 0; j < 3; ++j)
                {
                    printf("|%-5s", " ");
                    printf("\r\n");
                    MOVERIGHT(i * 7);
                }
            }

            for(int j = 0; j < 5; ++j)
            {
                printf("|%-5s", " ");
                printf("\r\n");
                MOVERIGHT(i * 7);
            }

            printf("|");
            for(int j = 0; j < 6; ++j)
            {
                printf("_");
            }
        }
        else
        {
            MOVEUP(11);
            for(int j = 0; j < 14; ++j)
            {
                printf("_");
            }
            printf("\r\n");
            MOVERIGHT(i * 7);

            if(bIsJoker)
            {
                printf("|%s%-14s%s|", joker_color.c_str(), "Ｊ", CLOSE_ATTR);

                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%s%-14s%s|", joker_color.c_str(), "Ｏ", CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%s%-14s%s|", joker_color.c_str(), "Ｋ", CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%s%-14s%s|", joker_color.c_str(), "Ｅ", CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%s%-14s%s|", joker_color.c_str(), "Ｒ", CLOSE_ATTR);
                printf("\r\n");
                MOVERIGHT(i * 7);

                for(int j = 0; j < 5; ++j)
                {
                    printf("|%-13s|", " ");
                    printf("\r\n");
                    MOVERIGHT(i * 7);
                }

                printf("|");
                for(int j = 0; j < 13; ++j)
                {
                    printf("_");
                }
                printf("|");
                printf("\r\n");
            }
            else
            {
                if(nPokerNumber == 10)
                {
                    printf("|%-13s|", print_number.c_str());
                }
                else
                {
                    printf("|%-14s|", print_number.c_str());
                }
                
                printf("\r\n");
                MOVERIGHT(i * 7);

                printf("|%-18s|", print_color.c_str());
                printf("\r\n");
                MOVERIGHT(i * 7);

                for(int j = 0; j < 8; ++j)
                {
                    printf("|%-13s|", " ");
                    printf("\r\n");
                    MOVERIGHT(i * 7);
                }

                printf("|");
                for(int j = 0; j < 13; ++j)
                {
                    printf("_");
                }
                printf("|");
                printf("\r\n");
            }
        }
    }
}

class B;

class A
{
public:
    A(){
        printf("construct A.\n");
    }

    ~A(){
        printf("destruct A.\n");
    }

    //std::shared_ptr<B> m_spb;
    std::weak_ptr<B> m_spb;
};

class B
{
public:
    B(){
        printf("construct B.\n");
    }

    ~B(){
        printf("destruct B.\n");
    }

    //std::shared_ptr<A> m_spa;
    std::weak_ptr<A> m_spa;
};

int main(int argc, char const *argv[])
{
	/*
	cout<< has_member_foo11<MyStruct>::value << endl;
	cout<< has_member_foo11<MyStruct, int>::value << endl;
	cout<< has_member_foo11<MyStruct, double>::value << endl;
	cout << factorial<5>::value << endl;  // constexpr (no calculations on runtime)
	static_assert(has_member_foo11<MyStruct>::value, "MyStruct has foo");
	static_assert(has_member_foo11<MyStruct, int>::value, "MyStruct has foo");
	*/
    /*CGame game;
    game.init();
    game.start();*/
    //picture_tagging();
    
    std::string heart("♥️");
    std::string spade("♠️");
    std::string club("♣️");
    std::string diamond("♦️");
    std::cout << heart << " size = " <<  heart.size() <<endl;
    std::cout << spade << " size = " <<  spade.size() <<endl;
    std::cout << club << " size = " <<  club.size() <<endl;
    std::cout << diamond << " size = " <<  diamond.size() <<endl;

    std::vector<std::string> vec_poker_number = {
        "Ａ",
        "１",
        "２",
        "３",
        "４",
        "５",
        "６",
        "７",
        "８",
        "９",
        "10",
        "Ｊ",
        "Ｑ",
        "Ｋ"
    };

    std::vector<std::string> vec_poker_color = {
        "♠️",
        "♥️",
        "♣️",
        "♦️"
    };

    //print_poker_card();
    readJsonFile("city.json");
    //alibabaCloudDownload();
    return 0;
}
