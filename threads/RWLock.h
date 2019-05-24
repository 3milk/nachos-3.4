// Solution of 1st type read&write problem
#ifndef RWLOCK_H
#define RWLOCK_H

#include "synch.h"


class RWLock
{
public:
	RWLock();
	~RWLock();

	void rlock();
	void runlock();
	void wlock();
	void wunlock();

private:
	Lock* mutex;
	Semaphore* wrtLock;
	int readerCnt;
};

// reader and writer
extern int shareMem;
extern RWLock* rwlock;
extern Semaphore* blockParentRW;
void InitReadAndWrite();
void Reader(int id);
void Writer(int id);
void DeleReadAndWrite();

#endif
