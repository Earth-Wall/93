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

    //��ͷ
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

        WORD m_wFlag; //������
        WORD m_wLen; //���ݳ���
        DWORD m_dwSeq; //�����
        DWORD m_dwCrcCheck; //У��
        BYTE m_data[DATALEN]; //������
    }PACKAGE, *PPACKAGE;
#pragma pack()

    DWORD m_dwNextRecvSeq;//��һ����Ҫ����İ������
    DWORD m_dwNextSendSeq;//��һ������ĵ����

    typedef struct tagPkgInfo
    {
        tagPkgInfo() {}
        tagPkgInfo(BOOL bSended, time_t tm, PACKAGE pkg) :
            m_bSended(bSended), m_tmPkg(tm), m_pkg(pkg)
        {}
        BOOL m_bSended; //���Ƿ��Ѿ����͹���������͹������ж��Ƿ�ʱ
        time_t m_tmPkg; //���ϴη��͵�ʱ��
        PACKAGE m_pkg;  //��������
    }PKGINFO, *PPKGINFO;
    list<PKGINFO> m_lstSend; //���͵Ķ���
    CLock m_lckForLstSend; //���ڷ��Ͷ��е�ͬ��

    map<DWORD, PACKAGE> m_mpRecv; //���ܰ��Ķ���
    CByteStreamBuff m_bufRecv; //�������ݵĻ�����
    CLock m_lckForBufRecv; //���ڻ�������ͬ��

    HANDLE m_hRecv;
    HANDLE m_hSend;
    static DWORD WINAPI RecvThreadFunc(LPVOID lpParam);
    static DWORD WINAPI SendThreadFunc(LPVOID lpParam);
};

