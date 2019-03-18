#include "game.hpp"
#include <thread>
#include <chrono>
#include <list>
#include <stdio.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

CMap::CMap(){}

CMap::~CMap(){}

void CMap::init()
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

void CMap::refresh(std::string snake_color)
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

CFood::CFood(CMap& map): m_map(map), m_strColor(GREEN){}

CFood::~CFood(){}

void CFood::random_make()
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

CSnake::CSnake(CMap& map, CFood& food):
	m_map(map), m_food(food), m_strColor(RED){}

CSnake::~CSnake(){}

void CSnake::init()
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

void CSnake::move_up()
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

void CSnake::move_down()
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

void CSnake::move_left()
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

void CSnake::move_right()
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

void CSnake::move_forward()
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

void CSnake::move_core(int r_x, int r_y) //参数为相对移动距离
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

CGame::CGame(){}

CGame::~CGame(){}

void CGame::start(int port)
{
	m_bExit = false;
	std::thread multicast_thread(multicast_thread_func, port);
	multicast_thread.detach();
}

void CGame::over()
{
	m_bExit = true;
}

void CGame::add_client(G_Client client)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	m_mapGameOpt.insert(client, CTaskQueue<GameOpt>());
}

void CGame::add_gameopt(G_Client client, GameOpt opt)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	auto iter = m_mapGameOpt.find(client);
	if(iter != m_mapGameOpt.end())
	{
		iter->second.AddTask(opt);
	}
}

void CGame::send_frame_thread_func(int port)
{
	int res = 0;
    int sockfd = 0;
    struct sockaddr_in mcast_addr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    IF_EXIT(sockfd < 0, "socket");

    memset(&mcast_addr, 0, sizeof(mcast_addr));
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_addr.s_addr = inet_addr("224.0.0.100");/*设置多播IP地址*/
    mcast_addr.sin_port = htons(port);/*设置多播端口*/

    char msg[] = "muticast message";
    GameOpt temp_opt;
    TGameFrame	temp_frame;
    size_t	frame_cnt = 0;
    int i = 0;
    while(!m_bExit)
    {
    	i = 0;
    	for(auto iter = m_mapGameOpt.begin();
    		iter != m_mapGameOpt.end(); ++ iter)
    	{
    		if(iter->second.Try_GetTask(temp_opt) == false)
    		{
    			temp_opt = GameOpt::MOVE_FORWARD;
    		}

    		temp_frame.opt[i++] = temp_opt;
    	}

    	temp_frame.szFrameID = frame_cnt++;

        res = sendto(sockfd,
            &temp_frame,
            sizeof(temp_frame),
            0,
            (struct sockaddr*)(&mcast_addr),
            sizeof(mcast_addr));

        std::cout << "res = " << res << endl;
        std::this_thread::sleep_for(std::chrono::miliseconds(50));
    }
}

CGameServer::CGameServer(){}

CGameServer::~CGameServer(){}

bool CGameServer::add_game(G_GameID id, CGame& game)
{
	std::lock_guard<std::mutex> lck(m_mtx);
	auto res_pair = m_mapGame.insert(make_pair(id, game));
	game = res_pair->first;
	return res_pair->second;
}

bool get_game(G_GameID id, CGame& game)
{
	std::lock_guard<std::mutex> lck(m_mtx);
	auto iter = m_mapGame.find(id);
	if(iter != m_mapGame.end())
	{
		game = iter->second();
		return true;
	}

	return false;
}

CGameClient::CGameClient(){}

CGameClient::~CGameClient(){}

void CGameClient::init()
{
	m_map.init();
    m_food.random_make();
}

void CGameClient::start()
{
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
        r = select(0 + 1, &rfds, NULL, NULL, &tv); //0：监听标准输入，若r=1，说明标准输入可读，rfds中标准输入文件描述符会就绪
        if(r<0)
        {
            write(1,"select() error.\n",16);
            break;
        }
        if(r == 1)
            continue;
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
}

void CGameClient::recv_frame_thread_func(int port)
{
	int res = 0;
    int sockfd = 0;
    struct sockaddr_in local_addr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    IF_EXIT(sockfd < 0, "socket");

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(10010);

    //绑定
    res = ::bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr));
    IF_EXIT(res < 0, "bind");

    //设置本地回环许可
    int loop = 1;
    res = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    IF_EXIT(res < 0, "setsockopt");

    //加入多播组
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.100"); /*多播组IP地址*/
    mreq.imr_interface.s_addr = htonl(INADDR_ANY); /*加入的客服端主机IP地址*/

    //本机加入多播组
    res = setsockopt(sockfd,
        IPPROTO_IP,
        IP_ADD_MEMBERSHIP,
        &mreq,
        sizeof(mreq));
    IF_EXIT(res < 0, "setsockopt");

    char buffer[1024];
    socklen_t srvaddr_len = sizeof(local_addr);
    while(1)
    {
        memset(buffer, 0, 1024);
        res = recvfrom(sockfd,
            buffer,
            1024,
            0,
            (struct sockaddr*)(&local_addr),
            &srvaddr_len);

        std::cout << "recvfrom : " << buffer << endl;
    }

    //退出多播组
    res = setsockopt(sockfd,
        IPPROTO_IP,
        IP_DROP_MEMBERSHIP,
        &mreq,
        sizeof(mreq));
    IF_EXIT(res < 0, "setsockopt");
}

void CGameClient::refresh_thread_func()
{

}








