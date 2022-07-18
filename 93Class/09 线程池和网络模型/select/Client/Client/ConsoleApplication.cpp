// ConsoleApplicationTest1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
using namespace std;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

void InitWs2();
void UninitWs32();


int main()
{
    InitWs2();
    //1)
    SOCKET sockClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockClient == SOCKET_ERROR)
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
    int nRet = connect(sockClient, (sockaddr*)&si, sizeof(si));
    if (nRet == SOCKET_ERROR)
    {
        printf("连接服务器失败 \r\n");
        return 0;
    }
    else
    {
        printf("连接服务器成功 \r\n");
    }

    while (true)
    {
        char szBuff[MAXBYTE] = { 0 };
        std::cin >> szBuff;

        nRet = send(sockClient, szBuff, sizeof(szBuff), 0);
        if (nRet == SOCKET_ERROR)
        {
            printf("发送失败\r\n");
        }
        else
        {
            printf("发送成功 \r\n");
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