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

struct geordinate{
    int x;
    int y;
};

struct Geo{
    float lon;
    float lat;
};

void print_matrix()
{
    char buff[256] = { 0 };

    //预定义10组二维坐标
    Geo geo[10] = { 0 };
    int idx = 0;
    int len = 0;
    float read_lon, read_lat = 0;

    std::ifstream input_file("/Users/xiangyu/Desktop/意大利苍耳国内分布数据.txt");
    if(input_file.is_open()){
        input_file.seekg(0, input_file.end);
        len = input_file.tellg();
        printf("file length = %d\n", len);
        input_file.seekg(0, input_file.beg);

        input_file.ignore(32, '\n'); //忽略掉第一行数据
        len = input_file.tellg();
        printf("first line length = %d\n", len);
        std::string temp;
        while(std::getline(input_file, temp)){
            printf("***** temp = %s\n", temp.c_str());
            std::istringstream istr(temp);
            istr >> read_lon >> read_lat;
            geo[idx].lon = read_lon;
            geo[idx].lat = read_lat;
            ++idx;
            memset(buff, 0, 256);
        }

        input_file.close();
    }else{
        printf("read error: %d\n", errno);
        return;
    }

    for(int i = 0; i < 10; ++i){
        printf("%d. {%f, %f}\n", i, geo[i].lon, geo[i].lat);
    }

    float distance[10][10] = { 0 };// distance[0][1]：表示第0组坐标与第一组坐标的距离

    //计算距离
    for(int i = 0; i < 10; ++i){
        for(int j = 0; j < 10; ++j){
            if(i != j){
                distance[i][j] = sqrt((geo[i].lon - geo[j].lon) * (geo[i].lon - geo[j].lon) + (geo[i].lat - geo[j].lat) * (geo[i].lat - geo[j].lat));
            }
        }
    }

    printf("%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n",
        "pop ID", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10");
    printf("==============================================================================================================\n");
    for(int i = 0; i < 10; ++i){
        printf("%-10d", i + 1);
        for(int j = 0; j < 10; ++j){
            if(i != j){
                printf("%-10.4f", distance[i][j]);
            }else{
                printf("%-10s", "****");
            }
        }
        printf("\n");
    }
    printf("==============================================================================================================\n");

    //将结果输出到到txt文件
    std::ofstream output_file("/Users/xiangyu/Desktop/意大利苍耳国内分布数据_统计结果矩阵.txt");
    if(output_file.is_open())
    {
        sprintf(buff, "%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\r\n",
            "pop ID", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10");
        std::string line("==============================================================================================================");
        output_file << std::string(buff) << "\r\n"; //写第一行
        output_file << line << "\r\n";
        for(int i = 0; i < 10; ++i){
            sprintf(buff, "%-10d", i + 1);
            output_file << std::string(buff);
            for(int j = 0; j < 10; ++j){
                if(i != j){
                    sprintf(buff, "%-10.4f", distance[i][j]);
                    output_file << std::string(buff);
                }else{
                    sprintf(buff, "%-10s", "****");
                    output_file << std::string(buff);
                }
            }
            output_file << "\r\n";
        }
        output_file << line;
        output_file.close();
    }
}

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
                ifs.ignore(10240, '\n');
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

int main(int argc, char const *argv[])
{
	/*
	cout<< has_member_foo11<MyStruct>::value << endl;
	cout<< has_member_foo11<MyStruct, int>::value << endl;
	cout<< has_member_foo11<MyStruct, double>::value << endl;
	cout << factorial<5>::value << endl;  // constexpr (no calculations on runtime)
	static_assert(has_member_foo11<MyStruct>::value, "MyStruct has foo");
	static_assert(has_member_foo11<MyStruct, int>::value, "MyStruct has foo");

	cout << std::boolalpha;
	cout << "is_same:" << std::endl;
	cout << "int, const int: " << std::is_same<int, const int>::value << std::endl;
	cout << "int, integer_type: " << std::is_same<int, integer_type>::value << std::endl;
	cout << "A, B: " << std::is_same<A,B>::value << std::endl;
	cout << "A, C: " << std::is_same<A,C>::value << std::endl;
	cout << "signed char, std::int8_t: " << std::is_same<signed char,std::int8_t>::value << std::endl;
	*/
    /*CGame game;
    game.init();
    game.start();*/
    //readFileList("/Users/xiangyu/Desktop/文档/图片标注/data1");
    picture_tagging();
    return 0;
}
