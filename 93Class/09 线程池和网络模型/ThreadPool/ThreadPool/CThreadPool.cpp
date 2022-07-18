#include "CThreadPool.h"

BOOL CThreadPool::CreateThreadPool(DWORD dwThreadCnt)
{
    //�����ź���
    m_hSemForTask = CreateSemaphore(NULL, 0, MAXLONG, NULL);
    if (m_hSemForTask == NULL)
    {
        return FALSE;
    }


    //����ִ��������߳�
    m_bRunning = TRUE;
    for (int i = 0; i < dwThreadCnt; ++i)
    {
      HANDLE hThread = CreateThread(
          NULL, 
          0, 
          ExcuteThreadProc, 
          this, 
          0, 
          NULL);
      if (hThread == NULL)
      {
          Destroy();
          return FALSE;
      }

      m_vctHandles.push_back(hThread);
    }

    return 0;
}

BOOL CThreadPool::AddTask(ITask* pTask)
{
    m_lckForQue.Lock();
    m_queTasks.push(pTask);
    ReleaseSemaphore(m_hSemForTask, 1, NULL);
    m_lckForQue.UnLock();
    return 0;
}

DWORD CALLBACK CThreadPool::ExcuteThreadProc(LPVOID lpParam)
{
    CThreadPool* pThis = (CThreadPool*)lpParam;

    while (pThis->m_bRunning)
    {
        //�ȴ�����
        WaitForSingleObject(pThis->m_hSemForTask, INFINITE);

        //�Ӷ�����ȡ������
        pThis->m_lckForQue.Lock();
        auto pTask = pThis->m_queTasks.front();
        pThis->m_queTasks.pop();
        pThis->m_lckForQue.UnLock();

        //ִ������
        pTask->ExcuteTask();
        
        //ɾ��ִ����ϵ�����
        delete pTask;
    }

    return TRUE;
}

BOOL CThreadPool::Destroy()
{
    m_bRunning = FALSE;
    CloseHandle(m_hSemForTask);
    WaitForMultipleObjects(
        m_vctHandles.size(), 
        m_vctHandles.data(), 
        TRUE, 
        INFINITE);

    for (auto hThread:m_vctHandles)
    {
        CloseHandle(hThread);
    }

    return TRUE;
}
