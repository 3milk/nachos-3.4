#include "ProducerAndConsumer.h"
#include "system.h"


bool buf[BUF_SIZE]; // true means full, false means empty
Semaphore* blockParent;
int nxtEmpty = 0;
int nxtFull = 0;

// implement by Semaphore
Semaphore* empty;
Semaphore* full;
Lock* bufMutex;

// implement by condition
Condition* emptyCon;
Condition* fullCon;
int emptyCnt;
Lock* conLock;


void InitPandC()
{
	empty = new Semaphore("empty", BUF_SIZE);
	full = new Semaphore("full", 0);
	bufMutex = new Lock("bufMutex");
	blockParent = new Semaphore("blockParent", 0);
	for(int i = 0; i<BUF_SIZE; i++)
		buf[i] = false;
}

void DelePandC()
{
	delete empty;
	delete full;
	delete bufMutex;
	delete blockParent;
}

void Producer(int times)
{
	printf("Produce times:%d \n", times);
	for(int i = 0; i<times; i++)
	{
		empty->P();
		bufMutex->Acquire();
		if(buf[nxtEmpty])
			printf("Err: Producer items:%d\n", i);
		buf[nxtEmpty] = true;
		printf("produce item:%d into pos:%d\n", i, nxtEmpty);
		nxtEmpty = (nxtEmpty+1)%BUF_SIZE;
		bufMutex->Release();
		full->V();
	}
}

void Consumer(int times)
{
	printf("Consume times:%d \n", times);
	for(int i = 0; i<times; i++)
	{
		full->P();
		bufMutex->Acquire();
		if(!buf[nxtFull])
			printf("Err: Consumer items:%d\n", i);
		buf[nxtFull] = false;
		printf("consume item:%d from pos:%d\n", i, nxtFull);
		nxtFull = (nxtFull+1)%BUF_SIZE;
		bufMutex->Release();
		empty->V();
	}
	blockParent->V();// JUST for 1 consumer
}


// implement by condition
void InitPandC_Condition()
{
	emptyCon = new Condition("emptyCon");
	fullCon = new Condition("fullCon");
	emptyCnt = BUF_SIZE;
	conLock = new Lock("conLock");
	blockParent = new Semaphore("blockParent", 0);
	for(int i = 0; i<BUF_SIZE; i++)
		buf[i] = false;
}

void DelePandC_Condition()
{
	delete emptyCon;
	delete fullCon;
	delete conLock;
	delete blockParent;
}

void Producer_Condition(int times)
{
	printf("Produce times:%d \n", times);
	for(int i = 0; i<times; i++)
	{
		conLock->Acquire();
		while(emptyCnt == 0)
			emptyCon->Wait(conLock);
		if(buf[nxtEmpty])
			printf("Err: Producer items:%d\n", i);
		buf[nxtEmpty] = true;
		emptyCnt--;
		printf("%s item:%d into pos:%d\n", currentThread->getName(), i, nxtEmpty);
		nxtEmpty = (nxtEmpty+1)%BUF_SIZE;
		fullCon->Signal(conLock);
		conLock->Release();
	}
}

void Consumer_Condition(int times)
{
	printf("Consume times:%d \n", times);
	for(int i = 0; i<times; i++)
	{
		conLock->Acquire();
		while(emptyCnt == BUF_SIZE)
			fullCon->Wait(conLock);
		if(!buf[nxtFull])
			printf("Err: Consumer items:%d\n", i);
		buf[nxtFull] = false;
		emptyCnt++;
		printf("%s item:%d from pos:%d\n", currentThread->getName(), i, nxtFull);
		nxtFull = (nxtFull+1)%BUF_SIZE;
		emptyCon->Signal(conLock);
		conLock->Release();
	}
	blockParent->V();
}
