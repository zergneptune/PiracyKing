#pragma once
#include "utility.hpp"
class CGameClient; 

int connect_server(char* server_ip, int server_port);

class CClientMng
{
public:
	CClientMng();
	~CClientMng();

public:
	void start_menu();

	int regist();

	int login();

	int logout();

	int join_room(uint64_t gid);

	int create_room(uint64_t& gid, std::string& strRoomName);

	int quit_room(uint64_t gid);

	int query_room_list(std::string& res);

	int query_room_players(uint64_t gid, std::string& res);

	int game_ready(uint64_t gid);

	int quit_game_ready(uint64_t gid);

	int recv_frame_ready(uint64_t gid);

	int quit_recv_frame_ready(uint64_t gid);

	int wait_to_start();

	void set_start(std::shared_ptr<TTaskData>& pTask);

	void send_game_player_cmd(std::shared_ptr<TTaskData>& pTask);

	int request_start_game(uint64_t gid);

	void game_start(uint64_t gid);

	void quit_game(uint64_t gid);

	int init(std::string ip, int port);

public:
	//测试
	void recv_muticast();

	void recv_broadcast();

private:
	
    int connect_server(std::string ip, int port);

    int init_thread();

	void socket_recv_thread_func();

	void socket_send_thread_func();

	void task_proc_thread_func();

private:

	int login_menu();

	int main_menu();

	int room_list_menu(uint64_t& nRoomId, std::string& strRoomName);

	int player_list_menu(uint64_t nRoomId, std::string& strRoomName);

private:
	int             m_nKq;

    int             m_nServerSockfd;

    int 			m_nClientID;
    
	SOCKETFD_QUE 	m_queSocketFD;

	TASK_QUE 		m_queTaskData;

	TASK_QUE 		m_queSendMsg;

	EVENT_NOTICE  	m_cEventNotice;

	CSnowFlake		m_cSnowFlake;

	std::mutex		m_mtx;

	std::condition_variable 	m_cv;

	bool			m_bGameStart;

private:
	CGameClient*	m_pGameClient;
};
