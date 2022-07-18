#pragma once
#include <windows.h>
#include <queue>
#include <vector>
using namespace std;

#include "./../../common/CLock.h"

class ITask
{
public:
    virtual void ExcuteTask() = 0;
};

class MyTask :public ITask
{
public:
	MyTask(int nVal) :m_nVal(nVal) {}
	void ExcuteTask() override
	{
		printf("val:%08X tid:%08X \r\n",
			m_nVal,
			GetCurrentThreadId());
	}

private:
	int m_nVal;
};

class CThreadPool
{
public:
    BOOL CreateThreadPool(DWORD dwThreadCnt);
    BOOL Destroy();
    BOOL AddTask(ITask* pTask);

private:
	queue<ITask*> m_queTasks; // �������
	CLock m_lckForQue;        // ��ͬ������
	HANDLE m_hSemForTask;     // �ź���

private:
    vector<HANDLE> m_vctHandles;
    BOOL m_bRunning;
    static DWORD CALLBACK ExcuteThreadProc(LPVOID lpParam);
};

