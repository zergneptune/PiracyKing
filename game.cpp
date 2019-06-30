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

GameOptType CSnake::get_move_direction()
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
        return GameOptType::MOVE_DOWN;
    }
    else if(x1 < x2)//向上移动
    {
        return GameOptType::MOVE_UP;
    }
    else if(y1 > y2)//向右移动
    {
        return GameOptType::MOVE_RIGHT;
    }
    else
    {
        return GameOptType::MOVE_LEFT;
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
    if(move_to_x < 0 || move_to_y < 0)
    {
        return;
    }
    
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
    if((*m_pMap).get_overlap(last_x, last_y) < 1)//如果蛇尾位置没有重叠数
    {
        (*m_pMap)[last_x][last_y] = MapType::BLANK;
    }
    iter->m_cox = move_to_x;
    iter->m_coy = move_to_y;
    m_snake.splice(m_snake.begin(), m_snake, iter);
}

void CSnake::vanish()
{
    std::lock_guard<std::mutex> lock(m_mt);
    for(auto iter = m_snake.begin(); iter != m_snake.end(); ++iter)
    {
        int cox = iter->m_cox;
        int coy = iter->m_coy;
        (*m_pMap)[cox][coy] = MapType::BLANK;
    }
}

CGame::CGame(G_GameID gameID, std::string strName):
    m_gameID(gameID), m_strRoomName(strName), m_roomOwner(-1), m_szGameFrameCnt(0), m_bIsGameRunning(false){}

CGame::~CGame(){}

void CGame::start(int port)
{
    //std::unique_lock<std::mutex> lck(m_mtx);
    //m_cv.wait(lck, [this](){ return is_all_ready_recv(); })
    std::this_thread::sleep_for(std::chrono::seconds(5));
	m_bExitSendFrame = false;
	std::thread send_frame_thread([this, &port]()
        {
            this->send_frame_thread_func(port);
        });
	send_frame_thread.detach();

    m_bIsGameRunning = true;
}

void CGame::over()
{
	m_bExitSendFrame = true;
    m_bIsGameRunning = false;
}

bool CGame::add_client(G_ClientID client)
{
	std::lock_guard<std::mutex> lock(m_mtx);
    if(m_mapGameOpt.size() == 2)
    {
        return false;
    }

	m_mapGameOpt.insert(std::make_pair(client,
                            std::make_shared<CTaskQueue<int>>()));

    m_mapPlayerStatus.insert(std::make_pair(client, TPlayerStatus()));
    
    return true;
}

bool CGame::remove_client(G_ClientID client)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto iter_1 = m_mapGameOpt.find(client);
    if(iter_1 != m_mapGameOpt.end())
    {
        m_mapGameOpt.erase(iter_1);
    }

    auto iter_2 = m_mapPlayerStatus.find(client);
    if(iter_2 != m_mapPlayerStatus.end())
    {
        m_mapPlayerStatus.erase(iter_2);
    }

    return true;
}

bool CGame::ready(G_ClientID client)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto iter = m_mapPlayerStatus.find(client);
    if(iter != m_mapPlayerStatus.end())
    {
        iter->second.m_nReadyGame = 1;
    }

    return true;
}

bool CGame::quit_ready(G_ClientID client)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto iter = m_mapPlayerStatus.find(client);
    if(iter != m_mapPlayerStatus.end())
    {
        if(iter->first != m_roomOwner) //房主默认始终处于准备状态
        {
            iter->second.m_nReadyGame = 0;
        }
    }

    return true;
}

bool CGame::get_ready_status(G_ClientID client)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto iter = m_mapPlayerStatus.find(client);
    if(iter != m_mapPlayerStatus.end())
    {
        return iter->second.m_nReadyGame;
    }

    return false;
}

bool CGame::get_running_status()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    int nPlayerNum = m_mapPlayerStatus.size();
    int nQuitReadyCnt = 0;
    for(auto iter = m_mapPlayerStatus.begin(); iter != m_mapPlayerStatus.end(); ++iter)
    {
        if(iter->second.m_nReadyGame == 0)
        {
            ++nQuitReadyCnt;
        }
    }

    if(nQuitReadyCnt == nPlayerNum - 1)
    {
        return false;
    }

    return true;
}

bool CGame::is_all_ready()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    for(auto iter = m_mapPlayerStatus.begin(); iter != m_mapPlayerStatus.end(); ++iter)
    {
        if(iter->second.m_nReadyGame == 0)
        {
            return false;
        }
    }

    return true;
}

void CGame::add_gameopt(G_ClientID client, GameOptType type)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	auto iter = m_mapGameOpt.find(client);
	if(iter != m_mapGameOpt.end())
	{
		iter->second->AddTask(static_cast<int>(type));
        printf("debug: 添加游戏操作 cid = %d, type = %d, 队列大小 = %d\n", client, type, iter->second->size());
	}
}

void CGame::get_game_frame(TGameFrameUdp& temp_frame)
{
    int i = 0;
    int opttype = 0;
    std::lock_guard<std::mutex> lock(m_mtx);
    for(auto iter = m_mapGameOpt.begin(); iter != m_mapGameOpt.end(); ++ iter)
    {
        if(iter->second->Try_GetTask(opttype) == false)
        {
            opttype = GameOptType::MOVE_FORWARD;
            printf("debug: 默认移动命令, 队列大小 = %d\n", iter->second->size());
        }
        else
        {
            printf("debug: cid = %d, opttype = %d, frame_cnt = %zu, 队列大小 = %d\n", iter->first, opttype, m_szGameFrameCnt, iter->second->size());
        }

        temp_frame.nClientID[i] = iter->first;
        temp_frame.optType[i] = opttype;
        ++i;
    }

    temp_frame.szFrameID = ++m_szGameFrameCnt;
    temp_frame.nGameId = m_gameID;
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
        vecCids.push_back(iter->first);
    }
}

int CGame::set_room_owner()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto iter = m_mapGameOpt.begin();
    if(iter != m_mapGameOpt.end())
    {
        m_roomOwner = iter->first;
        m_mapPlayerStatus[m_roomOwner].m_nReadyGame = 1; //房主默认准备
        return m_roomOwner;
    }

    return -1;
}

int CGame::set_room_owner(G_ClientID cid)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto iter = m_mapGameOpt.find(cid);
    if(iter != m_mapGameOpt.end())
    {
        m_roomOwner = cid;
        m_mapPlayerStatus[m_roomOwner].m_nReadyGame = 1; //房主默认准备
        return cid;
    }

    return -1;
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

    	temp_frame.szFrameID = ++ frame_cnt;

        res = sendto(sockfd,
            &temp_frame,
            sizeof(temp_frame),
            0,
            (struct sockaddr*)(&mcast_addr),
            sizeof(mcast_addr));

        //std::cout << "res = " << res << ", errno = " << errno << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    //关闭套接字
    close(sockfd);
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

uint64_t CGameServer::create_game(int cid, std::string& strGameName)
{
    uint64_t nNewGameId = m_cSnowFlake.get_sid();
	std::lock_guard<std::mutex> lck(m_mtx);
    if(is_game_name_existed(strGameName))
    {
        return -1;
    }

    auto res_pair = m_mapGame.insert(
    std::make_pair(nNewGameId, std::make_shared<CGame>(
                                            nNewGameId,
                                            strGameName)));
    if(res_pair.second)
    {
        //添加玩家
        res_pair.first->second->add_client(cid);
        //设置房主
        res_pair.first->second->set_room_owner();
        //房主默认游戏准备
        res_pair.first->second->ready(cid);
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
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        m_mapGame.erase(iter);
    }
}

void CGameServer::remove_player(int cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    for(auto iter = m_mapGame.begin(); iter != m_mapGame.end(); ++ iter)
    {
        if(iter->second->remove_client(cid))
        {
            if(iter->second->get_client_nums() == 0)
            {
                iter->second->over();
                m_mapGame.erase(iter);
                return;
            }
            else
            {
                //如果删除的玩家是房主，那么重新设置房主
                if(iter->second->get_room_owner() == cid)
                {
                    iter->second->set_room_owner();
                }
                return;
            }
        }
    }
}

void CGameServer::remove_player(G_GameID gid, int cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        if(iter->second->remove_client(cid))
        {
            if(iter->second->get_client_nums() == 0)
            {
                iter->second->over();
                m_mapGame.erase(iter);
            }
            else
            {
                //如果删除的玩家是房主，那么重新设置房主
                if(iter->second->get_room_owner() == cid)
                {
                    iter->second->set_room_owner();
                }
            }
        }
    }
}

int CGameServer::add_player(G_GameID gid, int cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        bool bRes = iter->second->add_client(cid);
        return (bRes ? 0 : -1);
    }
    
    return -2;
}

void CGameServer::add_game_player_opt(G_GameID gid, int cid, GameOptType type)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        iter->second->add_gameopt(cid, type);
    }
}

int CGameServer::game_ready(G_GameID gid, int cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        iter->second->ready(cid);
        return 0;
    }
    
    return -1;
}

int CGameServer::quit_game_ready(G_GameID gid, int cid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        iter->second->quit_ready(cid);
        return 0;
    }
    
    return -1;
}

void CGameServer::game_start(G_GameID gid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        iter->second->start(10010);
    }
}

void CGameServer::game_over(G_GameID gid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        iter->second->over();
    }
}

bool CGameServer::get_game_ready_status(G_GameID gid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        if(iter->second->is_all_ready())
        {
            return true;
        }
    }
    
    return false;
}

bool CGameServer::get_game_running_status(G_GameID gid)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(gid);
    if(iter != m_mapGame.end())
    {
        if(iter->second->get_running_status())
        {
            return true;
        }
    }
    
    return false;
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

void CGameServer::get_game_list(std::vector<std::shared_ptr<CGame>>& vecGames)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    for(auto iter = m_mapGame.begin(); iter != m_mapGame.end(); ++iter)
    {
        vecGames.push_back(iter->second);
    }
}

int CGameServer::get_room_owner(G_GameID id)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(id);
    if(iter != m_mapGame.end())
    {
        return iter->second->get_room_owner();
    }

    return -1;
}

int CGameServer::get_player_nums(G_GameID id)
{
    std::lock_guard<std::mutex> lck(m_mtx);
    auto iter = m_mapGame.find(id);
    if(iter != m_mapGame.end())
    {
        return iter->second->get_client_nums();
    }

    return -1;
}

CGameClient::CGameClient(TASK_QUE* p): m_pTaskData(p){}

CGameClient::~CGameClient(){}

void CGameClient::init()
{
    std::cout << "random seed = " << m_nRandSeed << std::endl;
	m_map.init();
    srand(m_nRandSeed);//设置随机种子
    m_QueGameCmd.clear();
    m_queGameFrame.clear();
    random_make_snake();
    m_map.random_make_food();
}

bool CGameClient::is_valid_move(int cid, GameOptType opttype)
{
    std::lock_guard<std::mutex> lck(m_mtx_snake);
    auto iter = m_mapSnake.find(cid);
    if(iter != m_mapSnake.end())
    {
        GameOptType curr_snake_move = iter->second->get_move_direction();
        if(opttype == GameOptType::MOVE_UP || opttype == GameOptType::MOVE_DOWN)
        {
            if(curr_snake_move == GameOptType::MOVE_LEFT || curr_snake_move == GameOptType::MOVE_RIGHT)
            {
                return true;
            }
        }
        else if(opttype == GameOptType::MOVE_LEFT || opttype == GameOptType::MOVE_RIGHT)
        {
            if(curr_snake_move == GameOptType::MOVE_UP || curr_snake_move == GameOptType::MOVE_DOWN)
            {
                return true;
            }
        }
    }

    return false;
}

void CGameClient::play(G_GameID gid, int cid, int port)
{
    init();//初始化
    //m_bExitRecvFrame = false;
    m_bExitRefresh = false;

    /*std::thread recv_frame_thread([this, port]()
        {
            this->recv_frame_thread_func(port);
        });
    recv_frame_thread.detach();*/

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
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    i = 0; q = 0; dir = 0;
    GameOptType opttype;
    while(1)
    {
        read(0, buf + i, 1);
        ++i;
        if(i > 31)
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
        else if(r == 1)
        {
            continue;
        }

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

            //if(is_valid_move(cid, opttype))
            //{
                printf("debug: 玩家操作 opttype = %d\n", opttype);
                m_pTaskData->AddTask(std::make_shared<TTaskData>(
                    MsgType::GAME_PLAYER_CMD,
                    gid,
                    cid,
                    static_cast<int>(opttype)));
            //}
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
    //m_bExitRecvFrame = true;
    m_bExitRefresh = true;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void CGameClient::add_snake(G_ClientID cid)
{
    std::lock_guard<std::mutex> lck(m_mtx_snake);
    m_mapSnake.insert(std::make_pair(cid,
                                    std::make_shared<CSnake>(&m_map)));
}

void CGameClient::remove_snake(G_ClientID cid)
{
    std::lock_guard<std::mutex> lck(m_mtx_snake);
    auto iter = m_mapSnake.find(cid);
    if(iter != m_mapSnake.end())
    {
        iter->second->vanish();
    }
}

void CGameClient::clear_snake()
{
    std::lock_guard<std::mutex> lck(m_mtx_snake);
    m_mapSnake.clear();
}

void CGameClient::random_make_snake()
{
    int cox, coy, value = 0;
    for(auto iter = m_mapSnake.begin(); iter != m_mapSnake.end(); ++ iter)
    {
        std::cout << "cid = " << iter->first << std::endl;
        while(1)
        {
            cox = 1 + rand() % (MAP_H - 2);
            coy = 1 + rand() % (MAP_W - 2);
            std::cout << "cox = " << cox << ", coy = " << coy << std::endl;
            value = m_map[cox][coy];
            if(value != MapType::SNAKE)
            {
                if(m_map[cox-1][coy] != MapType::SNAKE)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox-1][coy] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox-1, coy);
                    iter->second->add_node(CSnakeNode(cox, coy));
                    iter->second->add_node(CSnakeNode(cox-1, coy));
                    break;
                }
                else if(m_map[cox+1][coy] != MapType::SNAKE)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox+1][coy] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox+1, coy);
                    iter->second->add_node(CSnakeNode(cox, coy));
                    iter->second->add_node(CSnakeNode(cox+1, coy));
                    break;
                }
                else if(m_map[cox][coy-1] != MapType::SNAKE)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox][coy-1] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox, coy-1);
                    iter->second->add_node(CSnakeNode(cox, coy));
                    iter->second->add_node(CSnakeNode(cox, coy-1));
                    break;
                }
                else if(m_map[cox][coy+1] != MapType::SNAKE)
                {
                    m_map[cox][coy] = MapType::SNAKE;
                    m_map[cox][coy+1] = MapType::SNAKE;
                    m_map.inc_overlap(cox, coy);
                    m_map.inc_overlap(cox, coy+1);
                    iter->second->add_node(CSnakeNode(cox, coy));
                    iter->second->add_node(CSnakeNode(cox, coy+1));
                    break;
                }
            }
        }
    }
}

void CGameClient::add_game_frame(TGameFrameUdp* pGameFrame)
{
    if(pGameFrame)
    {
        m_queGameFrame.AddTask(
            std::make_shared<TGameFrameUdp>(*pGameFrame));
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
    //IF_EXIT(res < 0, "bind");

    //设置本地回环许可
    int loop = 1;
    res = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    IF_EXIT(res < 0, "setsockopt");

    //设置多播ip地址
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
    while(!m_bExitRecvFrame)
    {
        res = recvfrom(sockfd,
            buffer,
            sizeof(TGameFrame),
            0,
            (struct sockaddr*)(&local_addr),
            &srvaddr_len);
        IF_EXIT(res < 0, "recvfrom");

        //printf("recv res = %d\r\n", res);
        pframe = reinterpret_cast<TGameFrame*>(buffer);
        //m_queGameFrame.AddTask(std::make_shared<TGameFrame>(*pframe));
    }

    //退出多播组
    res = setsockopt(sockfd,
        IPPROTO_IP,
        IP_DROP_MEMBERSHIP,
        &mreq,
        sizeof(mreq));

    //关闭套接字
    close(sockfd);
    IF_EXIT(res < 0, "setsockopt");
}

void CGameClient::refresh_thread_func()
{
    G_ClientID cid;
    int opttype;
    while(!m_bExitRefresh)
    {
        std::shared_ptr<TGameFrameUdp> pframe = m_queGameFrame.Wait_GetTask();
        for(int i = 0; i < m_mapSnake.size(); ++i)
        {
            cid = pframe->nClientID[i];
            opttype = pframe->optType[i];
            printf("debug: cid = %d, recv opttype = %d\n", cid, opttype);
            auto iter = m_mapSnake.find(cid);
            if(iter != m_mapSnake.end())
            {
                switch(opttype)
                {
                    case GameOptType::MOVE_FORWARD:
                        iter->second->move_forward();
                        break;
                    case GameOptType::MOVE_UP:
                        iter->second->move_up();
                        break;
                    case GameOptType::MOVE_DOWN:
                        iter->second->move_down();
                        break;
                    case GameOptType::MOVE_LEFT:
                        iter->second->move_left();
                        break;
                    case GameOptType::MOVE_RIGHT:
                        iter->second->move_right();
                        break;
                    default:
                        break;
                }
            }
        }
        printf("\x1b[H\x1b[2J");
        m_map.refresh();
    }
}








