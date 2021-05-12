#include <iostream>
#include <map>
#include <WS2tcpip.h>
#include "..\..\Protocol\protocol.h"

using namespace std;
#pragma comment(lib, "WS2_32.LIB")

//서버로 보낼 데이터
typedef struct KeyInputs
{
	bool ARROW_UP = false;
	bool ARROW_DOWN = false;
	bool ARROW_LEFT = false;
	bool ARROW_RIGHT = false;
};

//서버에서 받을 데이터
typedef struct Pos
{
	int x = 3;
	int y = 3;
	int id = -1;
	short isplayer = 0;
};

struct SESSION
{
	WSAOVERLAPPED recv_overlapped;
	WSAOVERLAPPED send_overlapped;
	WSABUF dataBuffer;
	SOCKET socket;
	KeyInputs C_data; // 클라 -> 서버
	Pos S_data; // 서버 -> 클라
};

map <SOCKET, SESSION> clients;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

//이동 처리
void MovingPlayer(SESSION* client);

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	//do_recv();

	SOCKET client_s = reinterpret_cast<int>(over->hEvent);
	if (num_bytes == 0) {
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		cout << " 접속 종료" << endl;
		return;
	}
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	SOCKET client_s = reinterpret_cast<int>(over->hEvent);

	MovingPlayer(&clients[client_s]);
	cout << "Server Sent[" << client_s << "] : (" << clients[client_s].S_data.x <<", " << clients[client_s].S_data.y << ")" << endl;
	if (num_bytes == 0)
	{
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		cout << client_s << ": 접속 종료" << endl;
		return;
	}
	for (auto iter = clients.begin(); iter != clients.end(); iter++)
	{
		SOCKET sClient = iter->second.socket;
		if (sClient != client_s)//다른 플레이어들에게
		{
			clients[client_s].S_data.isplayer = 1;
			clients[sClient].dataBuffer.buf = (char*)&clients[client_s].S_data;
			clients[sClient].dataBuffer.len = sizeof(Pos);
			ZeroMemory(&(clients[sClient].send_overlapped), sizeof(WSAOVERLAPPED));
			clients[sClient].send_overlapped.hEvent = (HANDLE)clients[sClient].socket;
			WSASend(sClient, &(clients[sClient].dataBuffer), 1, NULL, 0, &(clients[sClient].send_overlapped), send_callback);
		}
		else//자기 자신 플레이어에게
		{
			clients[client_s].S_data.isplayer = 2;
			clients[sClient].dataBuffer.buf = (char*)&clients[client_s].S_data;
			clients[sClient].dataBuffer.len = sizeof(Pos);
			ZeroMemory(&(clients[sClient].send_overlapped), sizeof(WSAOVERLAPPED));
			clients[sClient].send_overlapped.hEvent = (HANDLE)clients[sClient].socket;
			WSASend(sClient, &(clients[sClient].dataBuffer), 1, NULL, 0, &(clients[client_s].send_overlapped), send_callback);
		}
	}

	clients[client_s].dataBuffer.buf = (char*)&clients[client_s].C_data;
	clients[client_s].dataBuffer.len = sizeof(KeyInputs);
	DWORD r_flag = 0;
	ZeroMemory(&(clients[client_s].recv_overlapped), sizeof(WSAOVERLAPPED));
	clients[client_s].recv_overlapped.hEvent = (HANDLE)clients[client_s].socket;
	WSARecv(client_s, &clients[client_s].dataBuffer, 1, 0, &r_flag, &(clients[client_s].recv_overlapped), recv_callback);
	cout << "Client Sent[" << client_s << "] : " << clients[client_s].C_data.ARROW_DOWN << clients[client_s].C_data.ARROW_UP << clients[client_s].C_data.ARROW_LEFT << clients[client_s].C_data.ARROW_RIGHT << endl;
}

void MovingPlayer(SESSION* client)
{
	if (client->C_data.ARROW_UP)
	{
		if (client->S_data.y > 0)
			client->S_data.y--;
	}
	else if (client->C_data.ARROW_DOWN)
	{
		if (client->S_data.y < 7)
			client->S_data.y++;
	}
	
	if (client->C_data.ARROW_LEFT)
	{
		if (client->S_data.x > 0)
			client->S_data.x--;
	}
	else if (client->C_data.ARROW_RIGHT)
	{
		if (client->S_data.x < 7)
			client->S_data.x++;
	}
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	
	SOCKADDR_IN svr_addr;
	memset(&svr_addr, 0, sizeof(SOCKADDR_IN));
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port = htons(SERVER_PORT);
	svr_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	bind(s_socket, reinterpret_cast<sockaddr *>(&svr_addr), sizeof(SOCKADDR_IN));
	listen(s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_len = sizeof(SOCKADDR_IN);
	memset(&cl_addr, 0, addr_len);

	int playerid = 0;

	while (true) {
		SOCKET clientSocket = accept(s_socket, (struct sockaddr*)&svr_addr, &addr_len);
		clients[clientSocket] = SESSION{};
		clients[clientSocket].socket = clientSocket;
		clients[clientSocket].S_data.id = playerid;
		clients[clientSocket].S_data.isplayer = 2;
		clients[clientSocket].dataBuffer.buf = (char*)&clients[clientSocket].S_data;
		clients[clientSocket].dataBuffer.len = sizeof(Pos);
		ZeroMemory(&(clients[clientSocket].send_overlapped), sizeof(WSAOVERLAPPED));
		clients[clientSocket].send_overlapped.hEvent = (HANDLE)clients[clientSocket].socket;
		ZeroMemory(&clients[clientSocket].recv_overlapped, sizeof(WSAOVERLAPPED));
		clients[clientSocket].recv_overlapped.hEvent = (HANDLE)clients[clientSocket].socket;
		DWORD r_flag = 0;
		
		WSASend(clients[clientSocket].socket, &(clients[clientSocket].dataBuffer), 1, NULL, 0, &(clients[clientSocket].send_overlapped), send_callback);
		cout << "New Client [" << clientSocket << "] connected" << endl;

		for (auto iter = clients.begin(); iter != clients.end(); iter++)
		{
			SOCKET sClient = iter->second.socket;
			if (sClient != clientSocket)
			{
				//다른 플레이어에게 접속한 플레이어를 알려줌
				clients[clientSocket].S_data.isplayer = 1;
				clients[sClient].dataBuffer.buf = (char*)&clients[clientSocket].S_data;
				clients[sClient].dataBuffer.len = sizeof(Pos);
				ZeroMemory(&(clients[sClient].send_overlapped), sizeof(WSAOVERLAPPED));
				clients[sClient].send_overlapped.hEvent = (HANDLE)clients[sClient].socket;
				WSASend(sClient, &(clients[sClient].dataBuffer), 1, NULL, 0, &(clients[sClient].send_overlapped), send_callback);


				//접속한 플레이어에게 모든 플레이어를 알려줌
				clients[sClient].S_data.isplayer = 1;
				clients[clientSocket].dataBuffer.buf = (char*)&clients[sClient].S_data;
				clients[clientSocket].dataBuffer.len = sizeof(Pos);
				ZeroMemory(&(clients[clientSocket].send_overlapped), sizeof(WSAOVERLAPPED));
				clients[clientSocket].send_overlapped.hEvent = (HANDLE)clients[clientSocket].socket;
				WSASend(clients[clientSocket].socket, &(clients[clientSocket].dataBuffer), 1, NULL, 0, &(clients[clientSocket].send_overlapped), send_callback);
			}
		}
		playerid++;
		clients[clientSocket].dataBuffer.buf = (char*)&clients[clientSocket].C_data;
		clients[clientSocket].dataBuffer.len = sizeof(KeyInputs);
		WSARecv(clients[clientSocket].socket, &clients[clientSocket].dataBuffer, 1, 0, &r_flag, &(clients[clientSocket].recv_overlapped), recv_callback);
		cout << "Client Sent[" << clientSocket << "] : " << clients[clientSocket].C_data.ARROW_DOWN << clients[clientSocket].C_data.ARROW_UP << clients[clientSocket].C_data.ARROW_LEFT << clients[clientSocket].C_data.ARROW_RIGHT << endl;
	}
	closesocket(s_socket);
	WSACleanup();
}