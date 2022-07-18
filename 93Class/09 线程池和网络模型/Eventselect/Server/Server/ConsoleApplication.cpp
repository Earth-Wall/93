// ConsoleApplicationTest1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <vector>
#include <map>
using namespace std;

#define FD_SETSIZE 128
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

void InitWs2();
void UninitWs32();
DWORD WINAPI SelectThreadFunc(LPVOID lpParam);

map<WSAEVENT, SOCKET> g_mpSockInfo;


int main()
{
    InitWs2();
    //1)
    SOCKET sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockServer == SOCKET_ERROR)
    {
        printf("socket 创建失败\r\n");
        return 0;
    }
    else
    {
        printf("socket 创建成功\r\n");
    }

    //2)
    sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    si.sin_port = htons(9527);
    int nRet = bind(sockServer, (sockaddr*)&si, sizeof(si));
    if (nRet == SOCKET_ERROR)
    {
        printf("绑定端口失败\r\n");
        return 0;
    }
    else
    {
        printf("绑定端口成功\r\n");
    }

    //3)
    nRet = listen(sockServer, SOMAXCONN);
    if (nRet == SOCKET_ERROR)
    {
        printf("监听失败 \r\n");
        return 0;
    }
    else
    {
        printf("监听成功 \r\n");
    }

    //这个socket只关心accept和close事件
    WSAEVENT hEvent = WSACreateEvent();
    WSAEventSelect(sockServer, hEvent, FD_ACCEPT | FD_CLOSE);
    g_mpSockInfo[hEvent] = sockServer;

    while (true)
    {
        WSAEVENT hEventAry[64] = {0};
        int nCount = 0;
        for (auto sockinfo:g_mpSockInfo)
        {
            hEventAry[nCount++] = sockinfo.first;
        }

        DWORD dwRet = WSAWaitForMultipleEvents(
            nCount, 
            hEventAry, 
            FALSE,
            INFINITE,
            FALSE);
        if (dwRet == WSA_WAIT_FAILED)
        {
            continue;
        }

        //取出socket
        SOCKET sock = g_mpSockInfo[hEventAry[dwRet]];
        WSANETWORKEVENTS event;
        WSAEnumNetworkEvents(sock, hEventAry[dwRet], &event);
        
        //判断
        if (event.lNetworkEvents & FD_ACCEPT)
        {
            sockaddr_in siClient;
            int nLen = sizeof(siClient);
            SOCKET sockClient = accept(
                sock,
                (sockaddr*)&siClient, &nLen);//建立连接的客户端的ip地址和端口号
            if (sockClient == INVALID_SOCKET)
            {
                printf("连接建立失败 \r\n");
                return 0;
            }
            else
            {
                printf("有新的客户端接入，%s:%d的 \r\n",
                    inet_ntoa(siClient.sin_addr),
                    ntohs(siClient.sin_port));
            }

            //这个socket只关心read和close事件
            WSAEVENT hEvent = WSACreateEvent();
            WSAEventSelect(sockClient, hEvent, FD_READ | FD_CLOSE);
            g_mpSockInfo[hEvent] = sockClient;
        }
        else if (event.lNetworkEvents & FD_READ)
        {
            char szBuff[MAXBYTE] = { 0 };
            int nRet = recv(sock, szBuff, sizeof(szBuff), 0);
            if (nRet == 0 || nRet == SOCKET_ERROR)
            {
                printf("接受数据失败\r\n");
            }
            else
            {
                printf("受到数据:%s \r\n", szBuff);
            }
        }
        else if (event.lNetworkEvents & FD_CLOSE)
        {
        }

    }

    UninitWs32();
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