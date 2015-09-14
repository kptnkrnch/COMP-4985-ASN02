#define STRICT

#include <winsock2.h>
#include <windows.h>
#include <WindowsX.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include "resource.h"
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

#define DEFPORT 5150
#define DEFPROT "TCP"
#define DEFSIZE 255
#define MAXSIZE 1024
#define DEFNUMP 10
#define DATA_BUFSIZE 8192
#define WM_SOCKET (WM_USER + 1)

typedef struct SOCKET_INFORMATION {
   BOOL RecvPosted;
   CHAR Buffer[DATA_BUFSIZE];
   WSABUF DataBuf;
   SOCKET Socket;
   DWORD BytesSEND;
   DWORD BytesRECV;
   SOCKET_INFORMATION *Next;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

typedef struct _SOCKET_INFORMATION {
   BOOL RecvPosted;
   CHAR Buffer[DATA_BUFSIZE];
   WSABUF DataBuf;
   SOCKET Socket;
   DWORD BytesSEND;
   DWORD BytesRECV;
   _SOCKET_INFORMATION *Next;
} _SOCKET_INFORMATION, * _LPSOCKET_INFORMATION;

_LPSOCKET_INFORMATION SocketInfoList;


typedef struct SEND_INFORMATION {
	char host[16];
	int port;
	bool TCP;
	bool UDP;
	int numPackets;
	int packetSize;
} SEND_INFORMATION, *LPSEND_INFORMATION;


TCHAR Name[] = TEXT("Assignment 02");
char str[80] = "";
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
#pragma warning (disable: 4096)

bool KillReader = false; //used for killing the Reading and Output thread
HANDLE opThrd; //Handle to the output thread
HANDLE PollingThrd; //Handle to the reading thread
HANDLE foundTag = CreateEvent(NULL, FALSE, FALSE, TEXT("RFID_Found")); //event to let the output thread know that a tag was found.
DWORD opThrdID;
DWORD PollingThrdID;
bool areaset = false;
int iVertPos = 0, iHorzPos = 0;
RECT text_area;
int mode = 0; // 0 = default, 1 = client, 2 = server
bool write = false;

DWORD WINAPI SendPacketThread(LPVOID);
DWORD WINAPI SendPacketThread2(LPVOID);
BOOL CALLBACK ToolBox(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ProtocolAndPort(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK IPConnect(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SendTestPackets(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SendFile(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

_LPSOCKET_INFORMATION GetSocketInformation(SOCKET s);

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hprevInstance,
 						  LPSTR lspszCmdParam, int nCmdShow)
{
	HWND hwnd;
	MSG Msg;
	WNDCLASSEX Wcl;

	Wcl.cbSize = sizeof (WNDCLASSEX);
	Wcl.style = CS_HREDRAW | CS_VREDRAW;
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION); // large icon 
	Wcl.hIconSm = NULL; // use small version of large icon
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW);  // cursor style
	
	Wcl.lpfnWndProc = WndProc;
	Wcl.hInstance = hInst;
	Wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH); //white background
	Wcl.lpszClassName = Name;
	
	Wcl.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN); // The menu Class
	Wcl.cbClsExtra = 0;      // no extra memory needed
	Wcl.cbWndExtra = 0; 
	
	if (!RegisterClassEx (&Wcl))
		return 0;

	hwnd = CreateWindow (Name, Name, WS_OVERLAPPEDWINDOW, 10, 10,
   							600, 400, NULL, NULL, hInst, NULL);

	ShowWindow (hwnd, nCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage (&Msg, NULL, 0, 0))
	{
   		TranslateMessage (&Msg);
		DispatchMessage (&Msg);
	}

	return Msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT Message,
                          WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	static int  cxClient,cyClient;
	SCROLLINFO si;
	PAINTSTRUCT ps;
	HINSTANCE hInst;
	HMENU menu;
	static LPSEND_INFORMATION info;
	static SOCKET client = INVALID_SOCKET;
	static SOCKET server = INVALID_SOCKET;
	WSADATA wsaData;
	_LPSOCKET_INFORMATION SocketInfo;
	DWORD RecvBytes, SendBytes;
	static bool SENDFLAG = false;
	DWORD test, errorz;
	struct sockaddr_in addr; 
	
	unsigned short totaltags;

	if (cxClient && cyClient) {
		text_area.left = 0;
		text_area.top = 0;
		text_area.right = cxClient;
		text_area.bottom = cyClient;
		areaset = true;
	}
	
	switch (Message)
	{
		case WM_CREATE:
			info = (LPSEND_INFORMATION)malloc(sizeof(SEND_INFORMATION));
			//hInst = GetModuleHandle(NULL);
			//CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hwnd, ToolBox);
			//menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CLIENT));
			//SetMenu(hwnd, menu);
		break;
		case WM_COMMAND:
			test = LOWORD (wParam);
			switch (test)
			{
				case CREATE_CLIENT:
					if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
						perror("WSAStartup failed.");
						break;
					}
					if (client = socket(AF_INET, SOCK_STREAM, 0) == INVALID_SOCKET) {
						perror("socket creation failed");
					}
					addr.sin_family = AF_INET; 
					addr.sin_addr.s_addr = inet_addr("192.168.0.104"); 
					addr.sin_port = htons(5150); 

					//WSAAsyncSelect(client, hwnd, WM_SOCKET, FD_CONNECT | FD_WRITE | FD_CLOSE);

					if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
						errorz = GetLastError();
						perror("error in connecting");
					}

				break;
				case IDM_CLIENT:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CLIENT));
					SetMenu(hwnd, menu);
					mode = 1;
				break;
				case IDM_SERVER:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_SERVER));
					SetMenu(hwnd, menu);
					mode = 2;
				break;
				case IDM_PROTPORT:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_PORTNPROT), hwnd, ProtocolAndPort);
				break;
				case IDM_CONNECT:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_IPCONNECT), hwnd, IPConnect);
				break;
				case IDM_SENDTEST:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_SENDTEST), hwnd, SendTestPackets);
				break;
				case IDM_SENDFILE:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_SENDFILE), hwnd, SendFile);
				break;
				case IDM_DISCONNECT:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MAIN));
					SetMenu(hwnd, menu);
					mode = 0;
					MessageBox(hwnd, TEXT("disconnected."), TEXT("ALERT"), MB_OK);
				break;
				case IDM_SENDPACKETS:
					strcpy(info->host, "192.168.0.104");
					info->numPackets = 1;
					info->packetSize = 16;
					info->port = 5150;
					info->TCP = true;
					info->UDP = false;
					PollingThrd = CreateThread(NULL, 0, SendPacketThread, (LPVOID)info, 0, &PollingThrdID);
					//PollingThrd = CreateThread(NULL, 0, SendPacketThread2, (LPVOID)&client, 0, &PollingThrdID);
				break;
				/*case IDM_Disconnect: //kills the reading thread and closes the com port if they are active
				break;*/
			}
		break;
		case WM_SIZE:
			cxClient = LOWORD(lParam);
			cyClient = HIWORD(lParam);

			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = cyClient;
			si.nPos = 0;
			si.nPage = 50;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

			break;
		case WM_VSCROLL:

			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hwnd, SB_VERT, &si);
			iVertPos = si.nPos;

			switch(LOWORD(wParam)){

			case SB_LINEUP:
				si.nPos -= 10;
				break;

			case SB_LINEDOWN:
				si.nPos += 10;
				break;

			case SB_PAGEUP:
				si.nPos -= si.nPage;
				break;

			case SB_PAGEDOWN:
				si.nPos += si.nPage;
				break;

			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;

			default:
				break;
			}

			si.fMask = SIF_POS;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
			GetScrollInfo(hwnd, SB_VERT, &si);

			//if there was change in the vertical scroll bar, make adjustments to redraw
			if (si.nPos != iVertPos){
				InvalidateRect(hwnd, &text_area, TRUE);
			}
		break;
		case WM_CHAR:	// Process keystroke

		break;
		
		case WM_DESTROY:	// Terminate program
      		PostQuitMessage (0);
		break;

		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			ReleaseDC(hwnd, hdc);
		break;
		case WM_SOCKET:
			if (WSAGETSELECTERROR(lParam)) {
				printf("Socket failed with error %d\n", WSAGETSELECTERROR(lParam));
			} else {
				switch(WSAGETSELECTEVENT(lParam)) {
					case FD_ACCEPT:

					break;
					case FD_READ:
					break;
					case FD_WRITE:  
						SENDFLAG = true;
						/*if (WSASend(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &SendBytes, 0, NULL, NULL) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSAEWOULDBLOCK) {
								printf("WSASend() failed with error %d\n", WSAGetLastError());
								return 0;
							}
						} else {
						}*/
					break;

					case FD_CLOSE:
						printf("Closing socket %d\n", wParam);
					break;
				}
			}
		break;
		default:
			return DefWindowProc (hwnd, Message, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI SendPacketThread(LPVOID sendinfo) {
	SEND_INFORMATION* info = (SEND_INFORMATION*)sendinfo;
	DWORD wait_result;
	
	int n, ns, bytes_to_read;
	int size = DEFSIZE;
	int numPackets = DEFNUMP;
	int port = 5150;
	int err;
	SOCKET sd;
	struct hostent	*hp;
	struct sockaddr_in server;
	char  *host, *bp, rbuf[MAXSIZE], sbuf[MAXSIZE], **pptr;
	std::string buffer = "";
	WSADATA WSAData;
	WORD wVersionRequested;
	bool TCP = true;

	wVersionRequested = MAKEWORD( 2, 2 );
	err = WSAStartup( wVersionRequested, &WSAData );
	if ( err != 0 ) //No usable DLL
	{
		printf ("DLL not found!\n");
		exit(1);
	}

	// Create the socket
	if (info != 0) {
		port = info->port;
		host = info->host;
		numPackets = info->numPackets;
		size = info->packetSize;

		if (info->TCP && info->UDP || !info->TCP && !info->UDP) {
			perror("Invalid protocol settings.");
		} else if (info->TCP) {
			TCP = true;
		} else {
			TCP = false;
		}
	}

	if (TCP) {
		if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			perror("Error occurred while creating the socket.");
			exit(1);
		}
	} else {
		if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			perror("Error occurred while creating the socket.");
			exit(1);
		}
	}
	//WSAAsyncSelect(sd, , WM_SOCKET, FD_WRITE | FD_CONNECT | FD_CLOSE);
	memset((char *)&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if ((hp = gethostbyname(host)) == NULL)
	{
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}

	// Copy the server address
	memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);

	if (TCP) {
		if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1)
		{
			fprintf(stderr, "Can't connect to server\n");
			perror("connect");
			exit(1);
		}
	}
	//pptr = hp->h_addr_list;
	memset((char *)sbuf, 0, sizeof(sbuf));

	for (int i = 0; i < size; i++) {
		buffer += "A";
	}

	for (int i = 0; i < 6; i++) {
		ns = send (sd, buffer.c_str(), buffer.size() + 1, 0);
	}
	closesocket(sd);

	return 0;
}

BOOL CALLBACK ProtocolAndPort(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	HWND tcp = 0, udp = 0;
    switch(Message)
    {
        case WM_INITDIALOG:
			tcp = GetDlgItem(hwnd, IDC_TCP);
			SendMessage(tcp, BM_SETCHECK, BST_CHECKED, 0);
        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				case IDC_TCP:
					tcp = GetDlgItem(hwnd, IDC_TCP);
					udp = GetDlgItem(hwnd, IDC_UDP);
					SendMessage(tcp, BM_SETCHECK, BST_CHECKED, 0);
					SendMessage(udp, BM_SETCHECK, BST_UNCHECKED, 0);
				break;
				case IDC_UDP:
					udp = GetDlgItem(hwnd, IDC_UDP);
					tcp = GetDlgItem(hwnd, IDC_TCP);
					SendMessage(udp, BM_SETCHECK, BST_CHECKED, 0);
					SendMessage(tcp, BM_SETCHECK, BST_UNCHECKED, 0);
				break;
                case IDOK:
                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK IPConnect(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	HWND parent;
    switch(Message)
    {
        case WM_INITDIALOG:

        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					parent = GetParent(hwnd);
					SendMessage(parent, WM_COMMAND, MAKEWPARAM(CREATE_CLIENT, CREATE_CLIENT), 0);
                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK SendTestPackets(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    //EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK SendFile(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	std::fstream ifs;
	OPENFILENAME ofn;
	static TCHAR szFilter[] = TEXT ("All Files (*.*)\0*.*\0\0") ;
	static TCHAR szFileName[MAX_PATH], szTitleName[MAX_PATH] ;
    switch(Message)
    {
        case WM_INITDIALOG:

        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
				case IDC_OPENFILE:
					ofn.lStructSize       = sizeof (OPENFILENAME) ;
					ofn.hwndOwner         = hwnd ;
					ofn.hInstance         = NULL ;
					ofn.lpstrFilter       = szFilter ;
					ofn.lpstrCustomFilter = NULL ;
					ofn.nMaxCustFilter    = 0 ;
					ofn.nFilterIndex      = 0 ;
					ofn.lpstrFile         = NULL ;          // Set in Open and Close functions
					ofn.nMaxFile          = MAX_PATH ;
					ofn.lpstrFileTitle    = NULL ;          // Set in Open and Close functions
					ofn.nMaxFileTitle     = MAX_PATH ;
					ofn.lpstrInitialDir   = NULL ;
					ofn.lpstrTitle        = NULL ;
					ofn.Flags             = 0 ;             // Set in Open and Close functions
					ofn.nFileOffset       = 0 ;
					ofn.nFileExtension    = 0 ;
					ofn.lpstrDefExt       = TEXT ("txt") ;
					ofn.lCustData         = 0L ;
					ofn.lpfnHook          = NULL ;
					ofn.lpTemplateName    = NULL ;

					ofn.hwndOwner         = hwnd ;
					ofn.lpstrFile         = szFileName ;
					ofn.lpstrFileTitle    = szTitleName ;
					ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;

					if (GetOpenFileName(&ofn)) {
						
						std::wstring wfilename(szFileName);
						std::string filename;
						HWND filenameBox;
						filenameBox = GetDlgItem(hwnd, IDC_FILENAME);

						SetWindowText(filenameBox, szFileName);

						for(int i = 0; i < wfilename.size(); i++) {
							filename += wfilename[i];
						}
						
						//ifs = OpenFile(filename);

						/*readFile(ifs);

						DWORD threadID;
						DWORD exitStatus;

						if (sendThread == 0 || (GetExitCodeThread(sendThread, &exitStatus) && exitStatus != STILL_ACTIVE)) {
							sendThread = CreateThread(NULL, 0, sendBufferThread, global, NULL, &threadID);
						}*/

					}
				break;
				case IDM_OPENFILE:
				break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

_LPSOCKET_INFORMATION GetSocketInformation(SOCKET s)
{
   _SOCKET_INFORMATION *SI = SocketInfoList;

   while(SI)
   {
      if (SI->Socket == s)
         return SI;

      SI = SI->Next;
   }

   return NULL;
}

DWORD WINAPI SendPacketThread2(LPVOID n) {
	SOCKET *s = (SOCKET*)n;
	DWORD SendBytes = 0;
	WSABUF buffer;
	std::string data = "AAAAA";
	buffer.len = data.size();
	buffer.buf = (CHAR*)data.c_str();
	WSASend(*s, &buffer, 1, &SendBytes, 0, NULL, NULL);
	return 0;
}
