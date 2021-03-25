#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <atlImage.h>

#include "resource.h"

using namespace std;
//서버 관련
#pragma comment(lib, "WS2_32.LIB")
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console" ) 

// 보드맵 사이즈
#define BOARD_SIZEX 8
#define BOARD_SIZEY 8 

// Server 관련
#define MAX_PLAYER 11
WSADATA WSAData;
SOCKET s_socket; // 서버 소켓
char serverip[32]; //서버아이피
constexpr short SERVER_PORT = 3500; // 서버 포트
WSAOVERLAPPED s_over;

//서버로 보낼 데이터
typedef struct KeyInputs
{
	bool ARROW_UP = false;
	bool ARROW_DOWN = false;
	bool ARROW_LEFT = false;
	bool ARROW_RIGHT = false;
}KeyInputs;

//서버에서 받을 데이터
typedef struct Pos
{
	int x = 3;
	int y = 3;
	short isplayer = 0;
}Pos;
KeyInputs C_data; // 서버로 보낼 데이터
Pos S_data; // 서버에서 받을 데이터
Pos Players[MAX_PLAYER];

// 윈도우 프로그래밍 관련
HINSTANCE g_hinst;
LPCTSTR lpszClass = "Window Class Name";
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM IParam);

//클라이언트 소켓 생성
void CreateClient(HWND hWnd);
//디버그 에러 출력
void display_error(const char* msg, int err_no);
//아이피 입력
BOOL CALLBACK DialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
//Recv Send 함수
void RecvSendData();
//Recv, Send 스레드 함수
DWORD WINAPI RecvSendMsg(LPVOID arg);
void do_send_message();
void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

BOOL CALLBACK DialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_IPADDRESS1, serverip, 100); // 스트링 복사
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
	wcout << L"에러 " << lpMsgBuf << endl;
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
	do_send_message();
}

void do_send_message()
{
	WSABUF s_wsabuf[1];
	s_wsabuf[0].buf = (char*)&C_data;
	s_wsabuf[0].len = sizeof(KeyInputs);
	memset(&s_over, 0, sizeof(s_over));
	WSASend(s_socket, s_wsabuf, 1, 0, 0, &s_over, send_callback);
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	cout << "Server Sent: " << S_data.x << S_data.y << endl;
	do_send_message();
}

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	C_data.ARROW_DOWN = false;
	C_data.ARROW_UP = false;
	C_data.ARROW_RIGHT = false;
	C_data.ARROW_LEFT = false;
	WSABUF r_wsabuf[1];
	r_wsabuf[0].buf = (char*)&S_data;
	r_wsabuf[0].len = sizeof(Pos);
	DWORD r_flag = 0;
	memset(over, 0, sizeof(*over));
	WSARecv(s_socket, r_wsabuf, 1, 0, &r_flag, over, recv_callback);
}

void RecvSendData()
{
	WSABUF s_wsabuf[1];
	s_wsabuf[0].buf = (char*)&C_data;
	s_wsabuf[0].len = sizeof(KeyInputs);
	DWORD sent_bytes;
	WSASend(s_socket, s_wsabuf, 1, &sent_bytes, 0, 0, 0);

	WSABUF r_wsabuf[1];
	r_wsabuf[0].buf = (char*)&S_data;
	r_wsabuf[0].len = sizeof(Pos);
	DWORD bytes_recv;
	DWORD r_flag = 0;
	int ret = WSARecv(s_socket, r_wsabuf, 1, &bytes_recv, &r_flag, 0, 0);
	if (SOCKET_ERROR == ret) {
		display_error("recv from server ", WSAGetLastError());
		exit(-1);
	}
}

DWORD WINAPI RecvSendMsg(LPVOID arg) 
{
	while (1) 
	{
		//RecvSendData()
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR IpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hinst = hinstance;
	//윈도우 클래스 구조체 값 설정
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

	//윈도우 클래스 등록
	RegisterClassEx(&WndClass);
	//윈도우 생성
	hWnd = CreateWindow(lpszClass, "게임서버", WS_OVERLAPPEDWINDOW, 0, 0, 600, 600, NULL, (HMENU)NULL, hinstance, NULL);

	//윈도우 출력
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	//클라이언트 소켓
	CreateClient(hWnd);
	//CreateThread(NULL, 0, RecvSendMsg, NULL, 0, NULL);
	//이벤트 루프 처리
	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
		SleepEx(100, true);
	}

	closesocket(s_socket);
	WSACleanup();
	return Message.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM IParam)
{
	PAINTSTRUCT ps;
	static CImage background; //보드맵
	static CImage chess; //플레이어

	static RECT rectView; //클라이언트 창 크기
	static HDC hDC,memDC;

	static int cx = 3, cy = 3; //플레이어 좌표
	static float dx, dy; //보드맵 한칸 크기
	static float mx, my; //마우스 좌표

	switch (iMessage) {
	case WM_CREATE:
	{
		srand((unsigned)time(NULL));
		GetClientRect(hWnd, &rectView);

		dx = (float)rectView.right / BOARD_SIZEX, dy = (float)rectView.bottom / BOARD_SIZEY;

		background.Load("img\\chessboard.jpg");
		chess.Load("img\\pngegg.png");
		return 0;
	}
	case WM_PAINT:
	{
		CImage img;
		img.Create(rectView.right, rectView.bottom, 24);
		hDC = BeginPaint(hWnd, &ps);

		cx = S_data.x;
		cy = S_data.y;

		memDC = img.GetDC();
		{
			Rectangle(memDC, 0, 0, rectView.right,rectView.bottom);
			background.Draw(memDC, 0, 0, rectView.right, rectView.bottom);
			chess.Draw(memDC, cx * dx, cy * dy, dx, dy);

			for (int i = 0; i < MAX_PLAYER; i++)
			{
				if (Players[i].isplayer == 1)
				{

				}
				else if (Players[i].isplayer == 2)
				{

				}
			}


			img.Draw(hDC, 0, 0);
			img.ReleaseDC();
		}
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			C_data.ARROW_LEFT = true;
			break;
		case VK_RIGHT:
			C_data.ARROW_RIGHT = true;
			break;
		case VK_UP:
			C_data.ARROW_UP = true;
			break;
		case VK_DOWN:
			C_data.ARROW_DOWN = true;
			break;
		}
		//RecvSendData();
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
		//RecvSendData();
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_CHAR:
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_LBUTTONDOWN: // 버튼을 누르면 드래그 동작 시작
	{
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	}
	case WM_LBUTTONUP: // 버튼을 놓으면 드래그 종료
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_RBUTTONDOWN:
	{
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	}
	case WM_MOUSEMOVE:
		break;

	case WM_TIMER: // 시간이 경과하면 메시지 자동 생성
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
	return(DefWindowProcA(hWnd, iMessage, wParam, IParam));//위의 세 메세지 외의 나머지 메세지는 OS로
}