#pragma once
#include < Winsock2.h>
#include <windows.h>
#include <list>
#include <map>
using namespace std;

#include "CLock.h"
#include "ByteStreamBuff.h"
#include "CCrc32.h"

class CUMTSocket
{
public:
    BOOL Accept(LPSTR szIP, WORD wPort);
    BOOL Connet(LPSTR szIp, WORD wPort);
    DWORD Recv(LPBYTE pBuff, DWORD dwBufSize);
    DWORD Send(LPBYTE pBuff, DWORD dwBufSize);
    VOID Close();

private:
    SOCKET m_sock;
    sockaddr_in m_si;

    //包头
    enum FLAG { SYN, PSH, ACK, FIN };
#define DATALEN 1460
#pragma pack(1)
    typedef struct tagPackge
    {
        tagPackge() {}
        tagPackge(WORD wFlag, DWORD dwSeq, LPBYTE pBuff = NULL, WORD nLen = 0) :
            m_wFlag(wFlag), m_dwSeq(dwSeq)
        {
            m_dwCrcCheck = 0;
            if (pBuff != NULL)
            {
                m_dwCrcCheck = CCrc32::crc32(pBuff, nLen);
                memcpy(m_data, pBuff, nLen);
                m_wLen = nLen;
            }
        }

        WORD m_wFlag; //包类型
        WORD m_wLen; //数据长度
        DWORD m_dwSeq; //包序号
        DWORD m_dwCrcCheck; //校验
        BYTE m_data[DATALEN]; //包数据
    }PACKAGE, *PPACKAGE;
#pragma pack()

    DWORD m_dwNextRecvSeq;//下一个需要处理的包的序号
    DWORD m_dwNextSendSeq;//下一个拆包的的序号

    typedef struct tagPkgInfo
    {
        tagPkgInfo() {}
        tagPkgInfo(BOOL bSended, time_t tm, PACKAGE pkg) :
            m_bSended(bSended), m_tmPkg(tm), m_pkg(pkg)
        {}
        BOOL m_bSended; //包是否已经发送过，如果发送过，则判断是否超时
        time_t m_tmPkg; //包上次发送的时间
        PACKAGE m_pkg;  //包的内容
    }PKGINFO, *PPKGINFO;
    list<PKGINFO> m_lstSend; //发送的队列
    CLock m_lckForLstSend; //用于发送队列的同步

    map<DWORD, PACKAGE> m_mpRecv; //接受包的队列
    CByteStreamBuff m_bufRecv; //接受数据的缓冲区
    CLock m_lckForBufRecv; //用于缓冲区的同步

    HANDLE m_hRecv;
    HANDLE m_hSend;
    static DWORD WINAPI RecvThreadFunc(LPVOID lpParam);
    static DWORD WINAPI SendThreadFunc(LPVOID lpParam);
};

