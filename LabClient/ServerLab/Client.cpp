#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <atlImage.h>

#include "resource.h"

#include "..\..\Protocol\protocol.h"

using namespace std;
//���� ����
#pragma comment(lib, "WS2_32.LIB")
//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console" ) 

constexpr auto BUF_SIZE = MAX_BUFFER;

// ����� ������
#define BOARD_SHOW_SIZEX 16
#define BOARD_SHOW_SIZEY 16
#define BOARD_SIZEX 16
#define BOARD_SIZEY 16

// Server ����
#define MAX_PLAYER 11
WSADATA WSAData;
SOCKET s_socket; // ���� ����
char serverip[32]; //����������
WSAOVERLAPPED s_over;

//������ ���� ������
typedef struct KeyInputs
{
	bool ARROW_UP = false;
	bool ARROW_DOWN = false;
	bool ARROW_LEFT = false;
	bool ARROW_RIGHT = false;
}KeyInputs;

//�������� ���� ������
typedef struct Pos
{
	int x = 3;
	int y = 3;
	bool bEnable = false;
}Pos;
KeyInputs C_data; // ������ ���� ������
Pos S_data; // �������� ���� ������
Pos Players[MAX_USER];

Pos myPlayer;
int myid;

// ������ ���α׷��� ����
HINSTANCE g_hinst;
LPCTSTR lpszClass = "Window Class Name";
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM IParam);

//Ŭ���̾�Ʈ ���� ����
void CreateClient(HWND hWnd);
//����� ���� ���
void display_error(const char* msg, int err_no);
//������ �Է�
BOOL CALLBACK DialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
//Send �Լ�
void SendData();
//Recv ������ �Լ�
DWORD WINAPI RecvSendMsg(LPVOID arg);

void send_login_packet();
void send_move_packet(int dir);
void process_data(char* net_buf, size_t io_byte);
void ProcessPacket(char* ptr);

BOOL CALLBACK DialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_IPADDRESS1, serverip, 100); // ��Ʈ�� ����
			EndDialog(hDlg, 0);
			break;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			exit(0);
			break;
		}
		break;
	}
	return 0;
}

void display_error(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err_no
		, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << L"���� " << lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

void CreateClient(HWND hWnd)
{
	wcout.imbue(locale("korean"));
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN svr_addr;
	memset(&svr_addr, 0, sizeof(svr_addr));
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port = htons(SERVER_PORT);
	DialogBox(g_hinst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)&DialogProc);
	inet_pton(AF_INET, serverip, &svr_addr.sin_addr);
	WSAConnect(s_socket, reinterpret_cast<sockaddr*>(&svr_addr), sizeof(svr_addr), NULL, NULL, NULL, NULL);

	send_login_packet();
	InvalidateRect(hWnd, NULL, FALSE);
}

void SendData()
{
	WSABUF s_wsabuf[1];
	s_wsabuf[0].buf = (char*)&C_data;
	s_wsabuf[0].len = sizeof(KeyInputs);
	DWORD sent_bytes;
	WSASend(s_socket, s_wsabuf, 1, &sent_bytes, 0, 0, 0);
}

DWORD WINAPI RecvSendMsg(LPVOID arg) 
{
	HWND hWnd = (HWND)arg;
	while (1) 
	{
		char net_buf[BUF_SIZE];

		WSABUF r_wsabuf[1];
		r_wsabuf[0].buf = (char*)&net_buf;
		r_wsabuf[0].len = BUF_SIZE;
		DWORD bytes_recv;
		DWORD r_flag = 0;
		auto recv_result = WSARecv(s_socket, r_wsabuf, 1, &bytes_recv, &r_flag, 0, 0);
		//Players[S_data.id] = S_data;
		//cout << "ServerSent [" << S_data.id << "] : " << S_data.x << ", " << S_data.y << "  isplayerd: " << S_data.isplayer << endl;

		if (bytes_recv > 0) process_data(net_buf, bytes_recv);

		InvalidateRect(hWnd, NULL, FALSE);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR IpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hinst = hinstance;
	//������ Ŭ���� ����ü �� ����
	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hinstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	//WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	//������ Ŭ���� ���
	RegisterClassEx(&WndClass);
	//������ ����
	hWnd = CreateWindow(lpszClass, "���Ӽ���", WS_OVERLAPPEDWINDOW, 0, 0, 600, 600, NULL, (HMENU)NULL, hinstance, NULL);

	//������ ���
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	//Ŭ���̾�Ʈ ����
	CreateClient(hWnd);
	CreateThread(NULL, 0, RecvSendMsg, (LPVOID)hWnd, 0, NULL); // Recv�� �����带 ������ �Լ� �ް� ��
	//�̺�Ʈ ���� ó��
	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	closesocket(s_socket);
	WSACleanup();
	return Message.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM IParam)
{
	PAINTSTRUCT ps;
	static CImage background; //�����
	static CImage chess; //�÷��̾�
	static CImage chess2; //�÷��̾�

	static RECT rectView; //Ŭ���̾�Ʈ â ũ��
	static HDC hDC,memDC;

	static int cx = 3, cy = 3; //�÷��̾� ��ǥ
	static float dx, dy; //����� ��ĭ ũ��
	static float mx, my; //���콺 ��ǥ

	switch (iMessage) {
	case WM_CREATE:
	{
		srand((unsigned)time(NULL));
		GetClientRect(hWnd, &rectView);

		dx = (float)rectView.right / BOARD_SIZEX, dy = (float)rectView.bottom / BOARD_SIZEY;

		background.Load("img\\chessboard.jpg");
		chess.Load("img\\pngegg.png");
		chess2.Load("img\\pngegg2.png");
		return 0;
	}
	case WM_PAINT:
	{
		CImage img;
		img.Create(rectView.right, rectView.bottom, 24);
		hDC = BeginPaint(hWnd, &ps);

		myPlayer.bEnable = true;

		cx = S_data.x;
		cy = S_data.y;
		memDC = img.GetDC();
		{
			Rectangle(memDC, 0, 0, rectView.right,rectView.bottom);
			//background.Draw(memDC, 0, 0, rectView.right, rectView.bottom);
			for (int y = 0; y < 50; y++)
			{
				for (int x = 0; x < 50; x++)
				{
					background.Draw(memDC
						, rectView.right / 2 * x - myPlayer.x * dx + BOARD_SIZEX / 2 * dx
						, rectView.bottom / 2 * y - myPlayer.y * dy + BOARD_SIZEY / 2 * dy
						, rectView.right / 2
						, rectView.bottom / 2);
				}
			}
			//chess.Draw(memDC, cx * dx, cy * dy, dx, dy);

			for (int i = 0; i < MAX_USER; i++)
			{
				if (Players[i].bEnable)
				{
					chess2.Draw(memDC
						, Players[i].x * dx - myPlayer.x * dx + BOARD_SIZEX / 2 * dx
						, Players[i].y * dy - myPlayer.y * dy + BOARD_SIZEX / 2 * dy
						, dx, dy);
				}
			}

			if (myPlayer.bEnable) {
				chess.Draw(memDC, BOARD_SIZEX / 2 * dx, BOARD_SIZEY / 2 * dy, dx, dy);
			}


			img.Draw(hDC, 0, 0);
			img.ReleaseDC();
		}
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_KEYDOWN:
		int dir;
		switch (wParam) {
		case VK_LEFT:
			C_data.ARROW_LEFT = true;
			dir = 3;
			break;
		case VK_RIGHT:
			C_data.ARROW_RIGHT = true;
			dir = 1;
			break;
		case VK_UP:
			C_data.ARROW_UP = true;
			dir = 0;
			break;
		case VK_DOWN:
			C_data.ARROW_DOWN = true;
			dir = 2;
			break;
		}
		send_move_packet(dir);
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_KEYUP:
		switch (wParam) {
		case VK_LEFT:
			C_data.ARROW_LEFT = false;
			break;
		case VK_RIGHT:
			C_data.ARROW_RIGHT = false;
			break;
		case VK_UP:
			C_data.ARROW_UP = false;
			break;
		case VK_DOWN:
			C_data.ARROW_DOWN = false;
			break;
		}
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_CHAR:
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_LBUTTONDOWN: // ��ư�� ������ �巡�� ���� ����
	{
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	}
	case WM_LBUTTONUP: // ��ư�� ������ �巡�� ����
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_RBUTTONDOWN:
	{
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	}
	case WM_MOUSEMOVE:
		break;

	case WM_TIMER: // �ð��� ����ϸ� �޽��� �ڵ� ����
		switch (wParam) {
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		}
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_COMMAND:
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_DESTROY:
		KillTimer(hWnd, 1);
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProcA(hWnd, iMessage, wParam, IParam));//���� �� �޼��� ���� ������ �޼����� OS��
}


void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case S2C_PACKET_LOGIN_INFO:
	{
		s2c_packet_login_info* packet = reinterpret_cast<s2c_packet_login_info*>(ptr);
		myid = packet->id;
		myPlayer.x = packet->x;
		myPlayer.y = packet->y;
		myPlayer.bEnable = true;
	}
	break;
	case S2C_PACKET_PC_LOGIN:
	{
		s2c_packet_pc_login* my_packet = reinterpret_cast<s2c_packet_pc_login*>(ptr);
		int id = my_packet->id;

		if (id < MAX_USER) {
			Players[id].x = my_packet->x;
			Players[id].y = my_packet->y;
			Players[id].bEnable = true;
		}
		else {
			//npc[id - NPC_START].x = my_packet->x;
			//npc[id - NPC_START].y = my_packet->y;
			//npc[id - NPC_START].attr |= BOB_ATTR_VISIBLE;
		}
		break;
	}
	case S2C_PACKET_PC_MOVE:
	{
		s2c_packet_pc_move* my_packet = reinterpret_cast<s2c_packet_pc_move*>(ptr);
		int other_id = my_packet->id;
		if (other_id == myid) {
			myPlayer.x = my_packet->x;
			myPlayer.y = my_packet->y;
		}
		else if (other_id < MAX_USER) {
			Players[other_id].x = my_packet->x;
			Players[other_id].y = my_packet->y;
		}
		else {
			//npc[other_id - NPC_START].x = my_packet->x;
			//npc[other_id - NPC_START].y = my_packet->y;
		}
		break;
	}

	case S2C_PACKET_PC_LOGOUT:
	{
		s2c_packet_pc_logout* my_packet = reinterpret_cast<s2c_packet_pc_logout*>(ptr);
		int other_id = my_packet->id;
		if (other_id == myid) {
			myPlayer.bEnable = false;
		}
		else if (other_id < MAX_USER) {
			Players[other_id].bEnable = false;
		}
		else {
			//		npc[other_id - NPC_START].attr &= ~BOB_ATTR_VISIBLE;
		}
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void send_move_packet(int dir)
{
	c2s_packet_move packet;
	packet.size = sizeof(packet);
	packet.type = C2S_PACKET_MOVE;
	packet.dir = dir;

	WSABUF s_wsabuf[1];
	s_wsabuf[0].buf = (char*)&packet;
	s_wsabuf[0].len = sizeof(packet);
	DWORD sent_bytes;
	WSASend(s_socket, s_wsabuf, 1, &sent_bytes, 0, 0, 0);
}

void send_login_packet()
{
	c2s_packet_login packet;
	packet.size = sizeof(packet);
	packet.type = C2S_PACKET_LOGIN;
	//strcpy_s(packet.name, to_string(rand() % 100).c_str());
	strcpy_s(packet.name, "nickname");

	WSABUF s_wsabuf[1];
	s_wsabuf[0].buf = (char*)&packet;
	s_wsabuf[0].len = sizeof(packet);
	DWORD sent_bytes;
	WSASend(s_socket, s_wsabuf, 1, &sent_bytes, 0, 0, 0);
}