#include "CUMTSocket.h"
#include <time.h>

BOOL CUMTSocket::Accept(LPSTR szIP, WORD wPort)
{
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET)
    {
        return FALSE;
    }

    sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_port = htons(wPort);
    si.sin_addr.S_un.S_addr = inet_addr(szIP);
    int nRet = bind(m_sock, (sockaddr*)&si, sizeof(si));
    if (nRet == SOCKET_ERROR)
    {
        closesocket(m_sock);
        return FALSE;
    }

    //三次握手
    PACKAGE pkg;
    sockaddr_in siClient;
    int nLen = sizeof(siClient);
     nRet = recvfrom(
        m_sock,
        (char*)&pkg, sizeof(pkg),
        0,
        (sockaddr*)&siClient, &nLen);
    if (nRet == 0 ||nRet == SOCKET_ERROR)
    {
        closesocket(m_sock);
        return FALSE;
    }
    m_si = siClient;
    
    m_dwNextRecvSeq = 0;
    if (pkg.m_wFlag != SYN && pkg.m_dwSeq == m_dwNextRecvSeq)
    {
        closesocket(m_sock);
        return FALSE;
    }
    m_dwNextRecvSeq++;

    m_dwNextSendSeq = 0;
    PACKAGE pkgSend(SYN, m_dwNextSendSeq);
    nRet = sendto(
        m_sock, 
        (char*)&pkgSend, sizeof(pkgSend), 
        0,
        (sockaddr*)&siClient, sizeof(siClient));
    if (nRet == 0)
    {
        closesocket(m_sock);
        return FALSE;
    }
    m_dwNextSendSeq++;

     nRet = recvfrom(
        m_sock,
        (char*)&pkg, sizeof(pkg),
        0,
        (sockaddr*)&siClient, &nLen);
    if (nRet == 0 || nRet == SOCKET_ERROR || pkg.m_dwSeq != m_dwNextRecvSeq)
    {
        closesocket(m_sock);
        return FALSE;
    }
    m_dwNextRecvSeq++;

    m_hRecv = CreateThread(NULL ,0, RecvThreadFunc, this, 0, NULL);
    m_hSend = CreateThread(NULL, 0, SendThreadFunc, this, 0, NULL);

    return TRUE;
}

BOOL CUMTSocket::Connet(LPSTR szIp, WORD wPort)
{
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET)
    {
        return FALSE;
    }

    sockaddr_in siServer;
    siServer.sin_family = AF_INET;
    siServer.sin_port = htons(wPort);
    siServer.sin_addr.S_un.S_addr = inet_addr(szIp);
    int nLen = sizeof(siServer);
    m_si = siServer;

    //三次握手
    //发送第一个包
    m_dwNextSendSeq = 0;
    PACKAGE pkg(SYN,  m_dwNextSendSeq);
    int nRet = sendto(
        m_sock,
        (char*)&pkg, sizeof(pkg),
        0,
        (sockaddr*)&siServer, sizeof(siServer));
    if (nRet == 0 || nRet == SOCKET_ERROR)
    {
        closesocket(m_sock);
        return FALSE;
    }
    m_dwNextSendSeq++;

    //从服务器接受第一个包
    m_dwNextRecvSeq = 0;
    PACKAGE pkgRecv;
     nRet = recvfrom(
        m_sock,
        (char*)&pkgRecv, sizeof(pkgRecv),
        0,
        (sockaddr*)&siServer, &nLen);
    if (nRet == 0 || nRet == SOCKET_ERROR || pkg.m_dwSeq != m_dwNextRecvSeq)
    {
        closesocket(m_sock);
        return FALSE;
    }
    m_dwNextRecvSeq++;

    //向服务器发送第二个包
    PACKAGE pkgSend(ACK, m_dwNextSendSeq);
    nRet = sendto(
        m_sock,
        (char*)&pkgSend, sizeof(pkgSend),
        0,
        (sockaddr*)&siServer, sizeof(siServer));
    if (nRet == 0)
    {
        closesocket(m_sock);
        return FALSE;
    }
    m_dwNextSendSeq++;

    m_hRecv = CreateThread(NULL, 0, RecvThreadFunc, this, 0, NULL);
    m_hSend = CreateThread(NULL, 0, SendThreadFunc, this, 0, NULL);

    return TRUE;
}

DWORD CUMTSocket::Recv(LPBYTE pBuff, DWORD dwBufSize)
{
    while (true)
    {
        m_lckForBufRecv.Lock();
        if (m_bufRecv.GetSize() <= 0)
        {
            //如果缓冲区中没有数据，则阻塞
            m_lckForBufRecv.UnLock();
            Sleep(1); //切出线程
            continue;
        }
        else
        {
            //如果缓冲区中有数据, 如果内部缓冲区中数据足够，则填满用户缓冲区，
            //如果内部缓冲区中数据不够，则有多少给多少
            DWORD dwRet = 
                m_bufRecv.GetSize() >= dwBufSize
                ? dwBufSize
                : m_bufRecv.GetSize();
            m_bufRecv.Read((char*)pBuff, dwRet);
            m_lckForBufRecv.UnLock();
            return dwRet;
        }
    }
}

DWORD CUMTSocket::Send(LPBYTE pBuff, DWORD dwBufSize)
{
    //计算拆包个数
    DWORD dwCount =
        dwBufSize % DATALEN == 0
        ? dwBufSize / DATALEN
        : (dwBufSize / DATALEN + 1);

    for (size_t i = 0; i < dwCount; i++)
    {
        //计算最后一个包的大小
        DWORD dwDataLen =
            (i == dwCount - 1)
            ? (dwBufSize - DATALEN * i)
            : DATALEN;

        //构造小包
        PACKAGE pkg(PSH, 
            m_dwNextSendSeq++, 
            pBuff + i*DATALEN, 
            dwDataLen);

        //小包进入队列
        m_lckForLstSend.Lock();
        m_lstSend.push_back(PKGINFO(FALSE, 0, pkg));
        m_lckForLstSend.UnLock();
    }
    return dwBufSize;
}

VOID CUMTSocket::Close()
{

}

DWORD WINAPI CUMTSocket::RecvThreadFunc(LPVOID lpParam)
{
    CUMTSocket* pThis = (CUMTSocket*)lpParam;
    while (true)
    {
        PACKAGE pkg;
        sockaddr sa;
        int nLen = sizeof(sa);
        int nRet = recvfrom(
            pThis->m_sock, 
            (char*)&pkg, sizeof(pkg),
            0,
            &sa,&nLen);
        if (nRet == 0 || nRet == SOCKET_ERROR)
        {
            continue;
        }

        //判断
        switch (pkg.m_wFlag)
        {
        case ACK:
        {
            //受到ack，从发送队列中删除对应seq的包
            pThis->m_lckForLstSend.Lock();
            for (auto itr = pThis->m_lstSend.begin();
                itr != pThis->m_lstSend.end();
                itr++)
            {
                if (pkg.m_dwSeq == itr->m_pkg.m_dwSeq)
                {
                    pThis->m_lstSend.erase(itr);
                    break;
                }
            }
            pThis->m_lckForLstSend.UnLock();
            break;
        }
        case PSH:
        {
            //包校验
            if (pkg.m_dwCrcCheck != CCrc32::crc32(pkg.m_data, pkg.m_wLen))
            {
                break;
            }

            //校验成功，包进链表
            pThis->m_mpRecv[pkg.m_dwSeq] = pkg;

            //回复ack
            PACKAGE pkgAck(ACK, pkg.m_dwSeq);
            sendto(
                pThis->m_sock, 
                (char*)&pkgAck, sizeof(pkgAck),
                0,
                (sockaddr*)&pThis->m_si, sizeof(pThis->m_si));

            break;
        }
        case FIN:
            break;
        default:
            break;
        }

        //将包中的数据放入缓冲区
        while (true)
        {
            //查找下一个序号的包是否在map中
            if (pThis->m_mpRecv.find(pThis->m_dwNextRecvSeq) != pThis->m_mpRecv.end())
            {
                //包中的数据放入缓冲区中
                pThis->m_lckForBufRecv.Lock();
                auto& pkg = pThis->m_mpRecv[pThis->m_dwNextRecvSeq];
                pThis->m_bufRecv.Write((char*)pkg.m_data, pkg.m_wLen);
                pThis->m_lckForBufRecv.UnLock();

                //将包删除
                pThis->m_mpRecv.erase(pThis->m_dwNextRecvSeq);

                //更新下一个需要处理的包的序号
                pThis->m_dwNextRecvSeq++;
            }
            else
            {
                break;
            }
        }

    }

    return 0;
}

DWORD WINAPI CUMTSocket::SendThreadFunc(LPVOID lpParam)
{
    CUMTSocket* pThis = (CUMTSocket*)lpParam;
    while (true)
    {
        pThis->m_lckForLstSend.Lock();
        for (auto& pi:pThis->m_lstSend)
        {
            //判断包是否发送过
            if (pi.m_bSended)
            {
                //已经发送过的包，判断是否超时
                if (time(NULL) - pi.m_tmPkg > 3)
                {
                    //发送超时的包
                    sendto(pThis->m_sock, 
                        (char*)&pi.m_pkg,
                        sizeof(pi.m_pkg),
                        0,
                        (sockaddr*)&pThis->m_si, sizeof(pThis->m_si));
                    pi.m_bSended = TRUE;
                    pi.m_tmPkg = time(NULL);
                }
            }
            else
            {
                //没有发送过的包发送出去
                sendto(pThis->m_sock,
                    (char*)&pi.m_pkg,
                    sizeof(pi.m_pkg),
                    0,
                    (sockaddr*)&pThis->m_si, sizeof(pThis->m_si));
                pi.m_bSended = TRUE;
                pi.m_tmPkg = time(NULL);
            }
        }
        pThis->m_lckForLstSend.UnLock();
    }

    return 0;
}

