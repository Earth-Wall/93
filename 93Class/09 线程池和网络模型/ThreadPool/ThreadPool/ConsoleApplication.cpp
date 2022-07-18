// ConsoleApplicationTest1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "CThreadPool.h"
using namespace std;


int main()
{
    CThreadPool tp;
    tp.CreateThreadPool(2);

    for (int i = 0; i < 0x1000; i++)
    {
        MyTask* pTask = new MyTask(i);
        tp.AddTask(pTask);
    }

    system("pause");
}