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

    //��������
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

    //��������
    //���͵�һ����
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

    //�ӷ��������ܵ�һ����
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

    //����������͵ڶ�����
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
            //�����������û�����ݣ�������
            m_lckForBufRecv.UnLock();
            Sleep(1); //�г��߳�
            continue;
        }
        else
        {
            //�����������������, ����ڲ��������������㹻���������û���������
            //����ڲ������������ݲ��������ж��ٸ�����
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
    //����������
    DWORD dwCount =
        dwBufSize % DATALEN == 0
        ? dwBufSize / DATALEN
        : (dwBufSize / DATALEN + 1);

    for (size_t i = 0; i < dwCount; i++)
    {
        //�������һ�����Ĵ�С
        DWORD dwDataLen =
            (i == dwCount - 1)
            ? (dwBufSize - DATALEN * i)
            : DATALEN;

        //����С��
        PACKAGE pkg(PSH, 
            m_dwNextSendSeq++, 
            pBuff + i*DATALEN, 
            dwDataLen);

        //С���������
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

        //�ж�
        switch (pkg.m_wFlag)
        {
        case ACK:
        {
            //�ܵ�ack���ӷ��Ͷ�����ɾ����Ӧseq�İ�
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
            //��У��
            if (pkg.m_dwCrcCheck != CCrc32::crc32(pkg.m_data, pkg.m_wLen))
            {
                break;
            }

            //У��ɹ�����������
            pThis->m_mpRecv[pkg.m_dwSeq] = pkg;

            //�ظ�ack
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

        //�����е����ݷ��뻺����
        while (true)
        {
            //������һ����ŵİ��Ƿ���map��
            if (pThis->m_mpRecv.find(pThis->m_dwNextRecvSeq) != pThis->m_mpRecv.end())
            {
                //���е����ݷ��뻺������
                pThis->m_lckForBufRecv.Lock();
                auto& pkg = pThis->m_mpRecv[pThis->m_dwNextRecvSeq];
                pThis->m_bufRecv.Write((char*)pkg.m_data, pkg.m_wLen);
                pThis->m_lckForBufRecv.UnLock();

                //����ɾ��
                pThis->m_mpRecv.erase(pThis->m_dwNextRecvSeq);

                //������һ����Ҫ����İ������
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
            //�жϰ��Ƿ��͹�
            if (pi.m_bSended)
            {
                //�Ѿ����͹��İ����ж��Ƿ�ʱ
                if (time(NULL) - pi.m_tmPkg > 3)
                {
                    //���ͳ�ʱ�İ�
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
                //û�з��͹��İ����ͳ�ȥ
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

