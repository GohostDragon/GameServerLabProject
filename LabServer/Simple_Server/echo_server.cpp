#include <iostream>
#include <map>
#include <WS2tcpip.h>

using namespace std;
#pragma comment(lib, "WS2_32.LIB")

constexpr short SERVER_PORT = 3500; // 서버 포트

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
};

struct SESSION
{
	WSAOVERLAPPED overlapped; // overlapped를 맨 위에 올린것은 SESSION 주소와 overlapped 주소와 똑같아 진다! (맨 앞에 있는 주소는 구조체 주소와 똑같다)
	WSABUF dataBuffer;
	SOCKET socket;
	KeyInputs C_data; // 클라한테 받을 데이터
	Pos S_data; // 클라한테 줄 데이터
};

map <SOCKET, SESSION> clients;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

//이동 처리
void MovingPlayer(SESSION* client);

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	//do_recv();

	SOCKET client_s = reinterpret_cast<SESSION*>(over)->socket;
	if (num_bytes == 0) {
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		cout << " 접속 종료" << endl;
		return;
	}  // 클라이언트가 closesocket을 했을 경우

	// WSASend(응답에 대한)의 콜백일 경우
	clients[client_s].dataBuffer.buf = (char*)&clients[client_s].C_data;
	clients[client_s].dataBuffer.len = sizeof(KeyInputs);
	DWORD r_flag = 0;
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));
	cout << "Client Sent[" << client_s << "] : " << clients[client_s].C_data.ARROW_DOWN << clients[client_s].C_data.ARROW_UP << clients[client_s].C_data.ARROW_LEFT << clients[client_s].C_data.ARROW_RIGHT << endl;;
	WSARecv(client_s, &clients[client_s].dataBuffer, 1, 0, &r_flag, over, recv_callback);
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	SOCKET client_s = reinterpret_cast<SESSION*>(over)->socket;

	MovingPlayer(&clients[client_s]);

	if (num_bytes == 0)
	{
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		cout << " 접속 종료" << endl;
		return;
	}
	clients[client_s].dataBuffer.buf = (char*)&clients[client_s].S_data;
	clients[client_s].dataBuffer.len = sizeof(Pos);
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));
	cout << "Server Send[" << client_s << "] : x: " << clients[client_s].S_data.x << "y: " << clients[client_s].S_data.y << endl;;
	WSASend(client_s, &(clients[client_s].dataBuffer), 1, NULL, 0, over, send_callback);
}

void MovingPlayer(SESSION* client)
{
	if (client->C_data.ARROW_UP)
	{
		if (client->S_data.y > 0)
			client->S_data.y--;
		client->C_data.ARROW_UP = false;
	}
	else if (client->C_data.ARROW_DOWN)
	{
		if (client->S_data.y < 7)
			client->S_data.y++;
		client->C_data.ARROW_DOWN = false;
	}
	
	if (client->C_data.ARROW_LEFT)
	{
		if (client->S_data.x > 0)
			client->S_data.x--;
		client->C_data.ARROW_LEFT = false;
	}
	else if (client->C_data.ARROW_RIGHT)
	{
		if (client->S_data.x < 7)
			client->S_data.x++;
		client->C_data.ARROW_RIGHT = false;
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
	while (true) {
		SOCKET clientSocket = accept(s_socket, (struct sockaddr*)&svr_addr, &addr_len);
		clients[clientSocket] = SESSION{};
		clients[clientSocket].socket = clientSocket;
		clients[clientSocket].dataBuffer.buf = (char*)&clients[clientSocket].C_data;
		clients[clientSocket].dataBuffer.len = sizeof(KeyInputs);
		memset(&clients[clientSocket].overlapped, 0, sizeof(WSAOVERLAPPED));
		DWORD r_flag = 0;
		WSARecv(clients[clientSocket].socket, &clients[clientSocket].dataBuffer, 1, 0, &r_flag, &(clients[clientSocket].overlapped), recv_callback);
		cout << "New Client [" << clientSocket << "] connected" << endl;
	}
	closesocket(s_socket);
	WSACleanup();
}