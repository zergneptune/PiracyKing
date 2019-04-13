#pragma once
#include "utility.hpp"
#include <functional>
class CGameServer;

int create_server(int port);

struct TClientInfo
{
	TClientInfo(int id, std::string account, std::string passwd, std::string name):
		nClientID(id), strAccount(account), strPasswd(passwd), strName(name){}

	int 			nClientID;

	std::string 	strAccount;

	std::string 	strPasswd;

	std::string 	strName;
};

class CClientInfoMng
{
	typedef	std::map<int, std::shared_ptr<TClientInfo>, std::less<int>> ClientInfoMap;
public:
	CClientInfoMng();
	~CClientInfoMng();

public:
	bool add_client(int cid, TClientInfo clientinfo);

	int get_max_client_id();

	bool is_client_existed(std::string& account);

	std::shared_ptr<TClientInfo> find_client(std::string& account);

	std::string get_name(int cid);

private:
	ClientInfoMap	m_mapClientInfo;

	std::mutex		m_mtx;
};

class COnlinePlayers
{
	typedef	int ClientID;
	typedef int SocketFd;
	typedef std::map<SocketFd, ClientID>	OnlinePlayersMap;

public:
	COnlinePlayers();
	~COnlinePlayers();

public:

	void add_player(SocketFd fd, ClientID cid);

	int remove_player_by_socketfd(SocketFd fd);

	void remove_player_by_clientid(ClientID id);

	bool is_player_online(ClientID cid);

	int get_sockfd(ClientID cid);

private:
	OnlinePlayersMap		m_mapOlinePlayers;

	std::mutex				m_mtx;
};

class CServerMng
{
public:
	CServerMng();
	~CServerMng();

public:
	void start(int port);

	void init();

	void send_muticast();

	void send_broadcast();

	void do_regiser(std::shared_ptr<TTaskData>& pTask);

	void do_login(std::shared_ptr<TTaskData>& pTask);

	void do_logout(std::shared_ptr<TTaskData>& pTask);

	void do_create_room(std::shared_ptr<TTaskData>& pTask);

	void do_join_room(std::shared_ptr<TTaskData>& pTask);

	void do_quit_room(std::shared_ptr<TTaskData>& pTask);

	void do_game_ready(std::shared_ptr<TTaskData>& pTask);

	void do_quit_game_ready(std::shared_ptr<TTaskData>& pTask);

	void do_game_start(std::shared_ptr<TTaskData>& pTask);

	void do_quit_game(std::shared_ptr<TTaskData>& pTask);

	void do_query_room_players(std::shared_ptr<TTaskData>& pTask);

	void do_request_game_start(std::shared_ptr<TTaskData>& pTask);

private:
	void init_thread();

	void socket_recv_thread_func();

	void socket_send_thread_func();

	void task_proc_thread_func();

	void listen_thread_func(int port);

private:
	int 			m_nPort;

	SOCKETFD_QUE 	m_queSockFD;

	TASK_QUE		m_queSendMsg;

	TASK_QUE 		m_queTaskData;

	CGameServer*	m_pGameServer;

	CClientInfoMng	m_ClientInfoMng;

	COnlinePlayers	m_COnlinePlayers;
};
