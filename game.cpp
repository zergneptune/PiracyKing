#include "game.hpp"
#include <thread>
#include <stdio.h>
#include <termios.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <json/json.h>

CMap::CMap(): m_strMapColor(BLACK), m_strFoodColor(GREEN){}

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

void CMap::refresh()
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
                    printf("%s*\033[m", m_strMapColor.c_str());
                    break;
                case MapType::SNAKE:
                    printf("%s@\033[m", RED);
                    break;
                case MapType::FOOD:
                    printf("%s$\033[m", m_strFoodColor.c_str());
                    break;
                default:
                    printf(" ");
            }
        }
        printf("\n");
    }
}

void CMap::random_make_food()
{
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

CSnake::CSnake(){}

CSnake::CSnake(CMap* pmap, std::string color):
	m_pMap(pmap), m_strColor(color){}

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
        value = (*m_pMap)[cox][coy];
        if(value != MapType::SNAKE && value != MapType::FOOD)
        {
            if((*m_pMap)[cox-1][coy] != MapType::SNAKE && (*m_pMap)[cox-1][coy] != MapType::FOOD)
            {
                (*m_pMap)[cox][coy] = MapType::SNAKE;
                (*m_pMap)[cox-1][coy] = MapType::SNAKE;
                (*m_pMap).inc_overlap(cox, coy);
                (*m_pMap).inc_overlap(cox-1, coy);
                m_snake.push_back(CSnakeNode(cox, coy));
                m_snake.push_back(CSnakeNode(cox-1, coy));
                break;
            }
            else if((*m_pMap)[cox+1][coy] != MapType::SNAKE && (*m_pMap)[cox+1][coy] != MapType::FOOD)
            {
                (*m_pMap)[cox][coy] = MapType::SNAKE;
                (*m_pMap)[cox+1][coy] = MapType::SNAKE;
                (*m_pMap).inc_overlap(cox, coy);
                (*m_pMap).inc_overlap(cox+1, coy);
                m_snake.push_back(CSnakeNode(cox, coy));
                m_snake.push_back(CSnakeNode(cox+1, coy));
                break;
            }
            else if((*m_pMap)[cox][coy-1] != MapType::SNAKE && (*m_pMap)[cox][coy-1] != MapType::FOOD)
            {
                (*m_pMap)[cox][coy] = MapType::SNAKE;
                (*m_pMap)[cox][coy-1] = MapType::SNAKE;
                (*m_pMap).inc_overlap(cox, coy);
                (*m_pMap).inc_overlap(cox, coy-1);
                m_snake.push_back(CSnakeNode(cox, coy));
                m_snake.push_back(CSnakeNode(cox, coy-1));
                break;
            }
            else if((*m_pMap)[cox][coy+1] != MapType::SNAKE && (*m_pMap)[cox][coy+1] != MapType::FOOD)
            {
                (*m_pMap)[cox][coy] = MapType::SNAKE;
                (*m_pMap)[cox][coy+1] = MapType::SNAKE;
                (*m_pMap).inc_overlap(cox, coy);
                (*m_pMap).inc_overlap(cox, coy+1);
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
    if((*m_pMap)[move_to_x][move_to_y] == MapType::BORDER)
    {
        return;
    }
    else if((*m_pMap)[move_to_x][move_to_y] == MapType::FOOD)
    {
        m_snake.push_front(CSnakeNode(move_to_x, move_to_y));
        (*m_pMap)[move_to_x][move_to_y] = MapType::SNAKE;
        (*m_pMap).inc_overlap(move_to_x, move_to_y);
        (*m_pMap).random_make_food();
        return;
    }

    (*m_pMap).inc_overlap(move_to_x, move_to_y);
    (*m_pMap)[move_to_x][move_to_y] = MapType::SNAKE;
    (*m_pMap).dec_overlap(last_x, last_y);//蛇尾离开，减少蛇尾位置重叠数
    if((*m_pMap).get_overlap(last_x, last_y) < 1)//如果蛇尾位置没有重叠
    {
        (*m_pMap)[last_x][last_y] = MapType::BLANK;
    }
    iter->m_cox = move_to_x;
    iter->m_coy = move_to_y;
    m_snake.splice(m_snake.begin(), m_snake, iter);
}

CGame::CGame(std::string strName): m_strRoomName(strName){}

CGame::~CGame(){}

void CGame::start(int port)
{
	m_bExitSendFrame = false;
	std::thread send_frame_thread([this, &port]()
        {
            this->send_frame_thread_func(port);
        });
	send_frame_thread.detach();
}

void CGame::over()
{
	m_bExitSendFrame = true;
}

bool CGame::add_client(G_Client client)
{
	std::lock_guard<std::mutex> lock(m_mtx);
    if(m_mapGameOpt.size() == 2)
    {
        return false;
    }

	m_mapGameOpt.insert(std::make_pair(client,
                            std::make_shared<CTaskQueue<int>>()));
    return true;
}

void CGame::remove_client(G_Client client)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    for(auto iter = m_mapGameOpt.begin();
                iter != m_mapGameOpt.end();
                ++ iter)
    {
        if(iter->first == client)
        {
            m_mapGameOpt.erase(iter);
            return;
        }
    }
}

void CGame::add_gameopt(G_Client client, GameOptType type)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	auto iter = m_mapGameOpt.find(client);
	if(iter != m_mapGameOpt.end())
	{
		iter->second->AddTask(static_cast<int>(type));
	}
}

int CGame::get_client_nums()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_mapGameOpt.size();
}

void CGame::get_client_ids(std::vector<int>& vecCids)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    for(auto iter = m_mapGameOpt.begin();
        iter != m_mapGameOpt.end();
        ++ iter)
    {
        std::cout << " CGame get cid = " << iter->first << std::endl;
        vecCids.push_back(iter->first);
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

    TGameFrame temp_frame;
    size_t frame_cnt = 0;
    int i = 0;
    int opttype = 0;
    while(!m_bExitSendFrame)
    {
    	i = 0;
    	for(auto iter = m_mapGameOpt.begin(); iter != m_mapGameOpt.end(); ++ iter)
    	{
    		if(iter->second->Try_GetTask(opttype) == false)
    		{
    			opttype = GameOptType::MOVE_FORWARD;
    		}

    		temp_frame.optType[i++] = opttype;
    	}

    	temp_frame.szFrameID = frame_cnt++;

        res = sendto(sockfd,
            &temp_frame,
            sizeof(temp_frame),
            0,
            (struct sockaddr*)(&mcast_addr),
            sizeof(mcast_addr));

        std::cout << "res = " << res << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

CGameServer::CGameServer(): m_cSnowFlake(0){}

CGameServer::~CGameServer(){}

bool CGameServer::is_game_name_existed(std::string& strGameName)
{
    for(auto iter = m_mapGame.begin(); iter != m_mapGame.end(); ++ iter)
    {
        if(iter->second->get_name() == strGameName)
        {
            return true;
        }
    }

    return false;
}

uint64_t CGameServer::create_game(std::string& strGameName)
{
    uint64_t nNewGameId = m_cSnowFlake.get_sid();
	std::lock_guard<std::mutex> lck(m_mtx);
    if(is_game_name_existed(strGameName))
    {
        return -1;
    }

    auto res_pair = m_mapGame.insert(
    std::make_pair(nNewGameId, std::make_shared<CGame>(
                                            strGameName)));
    if(res_pair.second)
    {
        return nNewGameId;
    }
    else
    {
        return -2;
    }
}

void CGameServer::remove_game(G_GameID gid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    m_mapGame.erase(m_mapGame.find(gid));
}

void CGameServer::remove_player(int cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    for(auto iter = m_mapGame.begin(); iter != m_mapGame.end(); ++ iter)
    {
        iter->second->remove_client(cid);
    }
}

std::shared_ptr<CGame> CGameServer::get_game(G_GameID gid)
{
	std::lock_guard<std::mutex> lck(m_mtx);
	auto iter = m_mapGame.find(gid);
	if(iter != m_mapGame.end())
	{
		return iter->second;
	}

	return std::shared_ptr<CGame>();
}

std::string CGameServer::get_gameid_list()
{
    Json::Value root;
    Json::Value value;
    Json::FastWriter fwriter;
    std::lock_guard<std::mutex> lck(m_mtx);
    for(auto iter = m_mapGame.begin(); iter != m_mapGame.end(); ++ iter)
    {
        value["gid"] = iter->first;
        value["gname"] = iter->second->get_name();
        value["players"] = iter->second->get_client_nums();
        root.append(value);
    }

    if(root)
    {
        return fwriter.write(root);
    }
    else
    {
        return std::string("{\"empty\"}");
    }
}

void CGameServer::get_cid_list(G_GameID id, std::vector<int>& vecCids)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(id);
    if(iter != m_mapGame.end())
    {
        iter->second->get_client_ids(vecCids);
    }
}

CGameClient::CGameClient(){}

CGameClient::~CGameClient(){}

void CGameClient::init()
{
	m_map.init();
    srand(m_nRandSeed);//设置随机种子
    random_make_snake();
    m_map.random_make_food();
}

void CGameClient::start(int port)
{
    init();//初始化
    m_bExitRecvFrame = false;
    m_bExitRefresh = false;

    std::thread recv_frame_thread([this, port]()
        {
            this->recv_frame_thread_func(port);
        });
    recv_frame_thread.detach();

    std::thread refresh_thread([this]()
        {
            this->refresh_thread_func();
        });
    refresh_thread.detach();

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
    int opttype = 0;
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
                    opttype = GameOptType::MOVE_UP;
                    break;
                case 0x42:
                    opttype = GameOptType::MOVE_DOWN;
                    break;
                case 0x43:
                    opttype = GameOptType::MOVE_RIGHT;
                    break;
                case 0x44:
                    opttype = GameOptType::MOVE_LEFT;
                    break;
                default:
                    break;
            }

            m_queGameCmd.AddTask(std::make_shared<TGameCmd>(
                m_gameID,
                m_clientID,
                opttype));
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

    //退出线程
    m_bExitRecvFrame = true;
    m_bExitRefresh = true;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void CGameClient::random_make_snake()
{
    int cox, coy, value = 0;
    for(auto iter = m_mapSnake.begin(); iter != m_mapSnake.end(); ++ iter)
    {
        while(1)
        {
            cox = 1 + rand() % (MAP_H - 2);
            coy = 1 + rand() % (MAP_W - 2);
            value = m_map[cox][coy];
            if(value != MapType::SNAKE)
            {
                if(m_map[cox-1][coy] != MapType::SNAKE)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox-1][coy] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox-1, coy);
                    iter->second.add_node(CSnakeNode(cox, coy));
                    iter->second.add_node(CSnakeNode(cox-1, coy));
                    break;
                }
                else if(m_map[cox+1][coy] != MapType::SNAKE)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox+1][coy] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox+1, coy);
                    iter->second.add_node(CSnakeNode(cox, coy));
                    iter->second.add_node(CSnakeNode(cox+1, coy));
                    break;
                }
                else if(m_map[cox][coy-1] != MapType::SNAKE)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox][coy-1] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox, coy-1);
                    iter->second.add_node(CSnakeNode(cox, coy));
                    iter->second.add_node(CSnakeNode(cox, coy-1));
                    break;
                }
                else if(m_map[cox][coy+1] != MapType::SNAKE)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox][coy+1] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox, coy+1);
                    iter->second.add_node(CSnakeNode(cox, coy));
                    iter->second.add_node(CSnakeNode(cox, coy+1));
                    break;
                }
            }
        }
    }
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
    local_addr.sin_port = htons(port);

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

    char buffer[sizeof(TGameFrame)];
    socklen_t srvaddr_len = sizeof(local_addr);
    TGameFrame* pframe = NULL;
    int i = 0;
    while(!m_bExitRecvFrame)
    {
        res = recvfrom(sockfd,
            buffer,
            sizeof(TGameFrame),
            0,
            (struct sockaddr*)(&local_addr),
            &srvaddr_len);
        IF_EXIT(res < 0, "recvfrom");

        pframe = reinterpret_cast<TGameFrame*>(buffer);
        m_queGameFrame.AddTask(std::make_shared<TGameFrame>(*pframe));
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
    int opttype;
    int i = 0;
    while(!m_bExitRefresh)
    {
        std::shared_ptr<TGameFrame> pframe = m_queGameFrame.Wait_GetTask();
        i = 0;
        for(auto iter = m_mapSnake.begin(); iter != m_mapSnake.end(); ++ iter)
        {
            opttype = pframe->optType[i++];
            switch(opttype)
            {
                case GameOptType::MOVE_FORWARD:
                    iter->second.move_forward();
                    break;
                case GameOptType::MOVE_UP:
                    iter->second.move_up();
                    break;
                case GameOptType::MOVE_DOWN:
                    iter->second.move_down();
                    break;
                case GameOptType::MOVE_LEFT:
                    iter->second.move_left();
                    break;
                case GameOptType::MOVE_RIGHT:
                    iter->second.move_right();
                    break;
                default:
                    break;
            }
        }
        m_map.refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}








