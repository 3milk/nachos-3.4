#include "RWLock.h"
#include "system.h"

int shareMem;
RWLock* rwlock;
Semaphore* blockParentRW;

RWLock::RWLock()
{
	mutex = new Lock("ReaderCnt mutex");
	wrtLock = new Semaphore("writer lock", 1);
	readerCnt = 0;
}


RWLock::~RWLock()
{
	delete mutex;
	delete wrtLock;
}

void
RWLock::rlock()
{
#ifdef FSTEST_MULTI_THREADS_READ_WRITE
	printf("%s try to aquire rlock...\n", currentThread->getName());
#endif
	mutex->Acquire();
	readerCnt++;
	if(readerCnt == 1)
		wrtLock->P();
	mutex->Release();
#ifdef FSTEST_MULTI_THREADS_READ_WRITE
	printf("%s aquired rlock! current readers cnt: %d\n", currentThread->getName(), readerCnt);
#endif
}

void
RWLock::runlock()
{
	mutex->Acquire();
	readerCnt--;
	if(readerCnt == 0)
		wrtLock->V();
	mutex->Release();
#ifdef FSTEST_MULTI_THREADS_READ_WRITE
	printf("%s released rlock~ current readers cnt: %d\n", currentThread->getName(), readerCnt);
#endif
}

void
RWLock::wlock()
{
#ifdef FSTEST_MULTI_THREADS_READ_WRITE
	printf("%s try to aquire wlock...\n", currentThread->getName());
#endif
	wrtLock->P();
#ifdef FSTEST_MULTI_THREADS_READ_WRITE
	printf("%s aquired wlock!\n", currentThread->getName());
#endif
}

void
RWLock::wunlock()
{
	wrtLock->V();
#ifdef FSTEST_MULTI_THREADS_READ_WRITE
	printf("%s released wlock~\n", currentThread->getName());
#endif
}


void InitReadAndWrite()
{
	shareMem = -1;
	rwlock = new RWLock();
	blockParentRW = new Semaphore("blockParentRW", 0);
}

void Reader(int id)
{
	for(int i = 0; i<5; i++) {
		rwlock->rlock();
		printf("Reader %d read %d\n", id, shareMem);
		rwlock->runlock();
		currentThread->Yield();
	}
	blockParentRW->V(); // finish reading
}

void Writer(int id)
{
	for(int i = 0; i<5; i++) {
		rwlock->wlock();
		shareMem = id + i;
		printf("Write %d write %d\n", id, shareMem);
		rwlock->wunlock();
		currentThread->Yield();
	}
	blockParentRW->V(); //finish writing
}

void DeleReadAndWrite()
{
	delete rwlock;
	delete blockParentRW;
}

