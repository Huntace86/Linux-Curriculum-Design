#pragma once
//此程序Win32和Linux都可以运行,Win32需要引用pthread-w32库
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#define dateTimeStringBufferSize (26)
#ifdef _WIN32//如果在VC下使用pthread-w32库测试代码的话 
#include <time.h>
#include <windows.h>//导入Sleep
#include <conio.h>//导入kbhit
#define clear ("cls")
#define gmtime_r(_Time,_Tm) ((gmtime_s(_Tm, _Time) == 0) ? (_Tm) : (NULL))
#define asctime_r(_Tm,_Buffer) ((asctime_s(_Buffer, dateTimeStringBufferSize, _Tm) == 0) ? (_Buffer) : (NULL))
#define usleep(x) (Sleep(x))
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif//_MSC_VER
#else
#ifdef __linux 
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#define clear ("clear")
int kbhit(void)//kbhit的Linux实现
{
	struct termios oldt, newt;
	int ch;
	int oldf;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	if (ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}
#endif // __linux 
#endif // _WIN32
///////////////////////////Time&String///////////////////////////
typedef struct tm DateTime;
typedef DateTime *RefDateTime;
typedef char* String;

String AllocString(int size)
{
	return malloc(sizeof(char) * size);
}

void FreeString(String str)
{
	free(str);
}

String DateTimeNowString()
{
	time_t nowtime = time(NULL);
	DateTime dataTime;
	if (gmtime_r(&nowtime,&dataTime) != NULL)
	{
		String buffer = AllocString(dateTimeStringBufferSize);
		if (asctime_r(&dataTime,buffer) != NULL)
		{
			return buffer;
		}
		else
		{
			free(buffer);
			return NULL;
		}
	}
	return NULL;
}

//////////////////////////Element//////////////////////////////////
pthread_mutex_t parkNumberBaseLock = PTHREAD_MUTEX_INITIALIZER;//车牌号增长的互斥量
size_t parkNumberBase = 0;

typedef struct ParkInfo
{
	size_t Number;//车牌号
	String Time;//入场时间
}ParkInfo;

typedef ParkInfo DataType;

typedef DataType *RefDataType;

typedef RefDataType *RefDataTypeArray;

RefDataType NewPark()
{
	RefDataType pdata = malloc(sizeof(DataType));
	pdata->Time = DateTimeNowString();
	pthread_mutex_lock(&parkNumberBaseLock);
	pdata->Number = (++parkNumberBase);//分配车牌号
	pthread_mutex_unlock(&parkNumberBaseLock);
	return pdata;
}

void FreePark(RefDataType pdate)
{
	FreeString(pdate->Time);
	free(pdate);
}

String DataToString(RefDataType pdata)
{
	String string = AllocString(128);
	sprintf(string,"车牌号: %zd 进场时间: %s", pdata->Number, pdata->Time);
	return string;
}

/////////////////////////Queue///////////////////////////////

typedef void(*DisplayAction)(RefDataType);

typedef struct Queue
{
	int Capacity;
	int Head;
	int Tail;
	RefDataTypeArray Elements;
}Queue, *RefQueue;

RefQueue NewQueue(int cap)
{
	if (cap <= 0)
	{
		return NULL;
	}
	RefQueue pqueue = malloc(sizeof(RefQueue));
	pqueue->Capacity = cap;
	pqueue->Head = 0;
	pqueue->Tail = 0;
	pqueue->Elements = malloc(sizeof(RefDataType)*(pqueue->Capacity + 1));
	return pqueue;
}

bool IsEmptyQueue(RefQueue pqueue)
{
	return pqueue->Tail == pqueue->Head;
}

bool IsFullQueue(RefQueue pqueue)
{
	return (pqueue->Tail + 1) % (pqueue->Capacity + 1) == pqueue->Head;
}

int QueueCount(RefQueue pqueue)
{
	return (pqueue->Tail - pqueue->Head + (pqueue->Capacity + 1)) % (pqueue->Capacity + 1);
}

bool EnQueue(RefQueue pqueue, RefDataType data)
{
	if (IsFullQueue(pqueue))
	{
		return false;
	}
	pqueue->Elements[pqueue->Tail] = data;
	pqueue->Tail = (pqueue->Tail + 1) % (pqueue->Capacity + 1);
	return true;
}

RefDataType DeQueue(RefQueue pqueue)
{
	if (IsEmptyQueue(pqueue))
	{
		return NULL;
	}
	RefDataType e = pqueue->Elements[pqueue->Head];
	pqueue->Head = (pqueue->Head + 1) % (pqueue->Capacity + 1);
	return e;
}

void FreeQueue(RefQueue pqueue)
{
	free(pqueue->Elements);
	free(pqueue);
}

void DisplayQueue(RefQueue pqueue, DisplayAction display)
{

	printf("--------------------------Begin display--------------------------\n");
	if (IsEmptyQueue(pqueue))
	{
		printf("--------------------------Park Empty-----------------------------\n");
		printf("---------------------------End display---------------------------\n");
		return;
	}
	printf("--------------------------Park count %d---------------------------\n",QueueCount(pqueue));
	int index = pqueue->Head;
	while ((index % (pqueue->Capacity+1)) != pqueue->Tail)
	{
		display(pqueue->Elements[(index++) % (pqueue->Capacity + 1)]);
	}
	printf("---------------------------End display---------------------------\n");
}

RefDataType GetQueueHead(RefQueue pqueue)
{
	if (IsEmptyQueue(pqueue))
	{
		return NULL;
	}
	return pqueue->Elements[pqueue->Head];
}

RefDataType GetQueueTail(RefQueue pqueue)
{
	if (IsEmptyQueue(pqueue))
	{
		return NULL;
	}
	return pqueue->Elements[pqueue->Tail];
}

//////////////////////////Character/////////////////////////////////
void*Producer(void*arg);

void*Consumer(void*arg);

void*Observer(void*arg);

//////////////////////////Main//////////////////////////////
#define delayTime (3)  //放车与取车之间的最大时间间隔
#define printTime (1)	//打印信息的间隔
#define parkCapcity (10)//停车场容量
#define producerCount (2)//生产者数量
#define consumerCount (2)//消费者数量
RefQueue parkQueue;//停车场队列
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;//访问队列的互斥量
pthread_cond_t producerCond = PTHREAD_COND_INITIALIZER; //Full->Not Full
pthread_cond_t consumerCond = PTHREAD_COND_INITIALIZER; //Empty->Not Empty

///////////////////////////Character///////////////////////////////////////

void*Producer(void*arg)
{
	pthread_detach(pthread_self());
	while (1)
	{
		pthread_mutex_lock(&queueLock);
		while (IsFullQueue(parkQueue))  //队列由满变为非满时，生产者线程被唤醒
		{
			pthread_cond_wait(&producerCond, &queueLock);
		}
		RefDataType pdata = NewPark();
		EnQueue(parkQueue, pdata); //入队
		if (QueueCount(parkQueue) == 1) //当生产者开始产出后，通知(唤醒)消费者线程
		{
			pthread_cond_broadcast(&consumerCond);
		}
		pthread_mutex_unlock(&queueLock);
		usleep(rand() % delayTime + 1);
	}
}

void*Consumer(void*arg)
{
	pthread_detach(pthread_self());
	while (1)
	{
		pthread_mutex_lock(&queueLock);
		while (IsEmptyQueue(parkQueue)) //队列由空变为非空时，消费者线程将被唤醒
		{
			pthread_cond_wait(&consumerCond, &queueLock);
		}
		FreePark(DeQueue(parkQueue));
		if (QueueCount(parkQueue) == (parkQueue->Capacity - 1)) //当队列由满变为非满时，通知(唤醒)生产者线程
		{
			pthread_cond_broadcast(&producerCond);
		}
		pthread_mutex_unlock(&queueLock);
		usleep(rand() % delayTime + 1);
	}
}

void ObserverPrint(const char*message)
{
	printf("[Observer]: %s\n", message);
}

void DisplayParkInfo(RefDataType pdata)
{
	if (pdata == NULL)
	{
		return;
	}
	String string = DataToString(pdata);
	ObserverPrint(string);
	FreeString(string);
}

void*Observer(void*arg)
{
	pthread_detach(pthread_self());
	while (1)
	{
		pthread_mutex_lock(&queueLock);
		DisplayQueue(parkQueue, DisplayParkInfo);
		pthread_mutex_unlock(&queueLock);
		usleep(printTime);
		system(clear);
	}
}