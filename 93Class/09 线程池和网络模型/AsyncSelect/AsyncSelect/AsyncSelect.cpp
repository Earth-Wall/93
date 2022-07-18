// AsyncSelect.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "AsyncSelect.h"

#define  WM_SOCK WM_USER+1

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    InitWs2();
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), NULL, About);
    UninitWs32();
    return (int) 0;
}




// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        SOCKET sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in si;
        si.sin_family = AF_INET;
        si.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        si.sin_port = htons(9527);
        int nRet = bind(sockServer, (sockaddr*)&si, sizeof(si));
        nRet = listen(sockServer, SOMAXCONN);

        WSAAsyncSelect(sockServer, hDlg, WM_SOCK, FD_ACCEPT|FD_CLOSE);

    }
        return (INT_PTR)TRUE;
    case WM_SOCK:
    {
        SOCKET sock = (SOCKET)wParam;
        WORD wEvent = WSAGETSELECTEVENT(lParam);

        if (wEvent & FD_ACCEPT)
        {
            sockaddr_in siClient;
            int nLen = sizeof(siClient);
            SOCKET sockClient = accept(
                sock,
                (sockaddr*)&siClient, &nLen);//建立连接的客户端的ip地址和端口号
           
            char szBuff[MAXBYTE] = {0};
            wsprintf(szBuff, "[AS]有新的客户端接入，%s:%d的 \r\n",
                inet_ntoa(siClient.sin_addr),
                ntohs(siClient.sin_port));
            OutputDebugString(szBuff);

            //这个socket只关心read和close事件
            WSAAsyncSelect(sockClient, hDlg, WM_SOCK, FD_READ | FD_CLOSE);

        }
        else if (wEvent & FD_READ)
        {
            char szBuff[MAXBYTE] = { 0 };
            int nRet = recv(sock, szBuff, sizeof(szBuff), 0);

            char szOutBuff[MAXBYTE] = { 0 };
            wsprintf(szOutBuff, "[AS]受到数据:%s \r\n", szBuff);
            OutputDebugString(szOutBuff);
        }

        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


void InitWs2()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        return;
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup();
        return;
    }

    /* The WinSock DLL is acceptable. Proceed. */
}

void UninitWs32()
{
    WSACleanup();
}