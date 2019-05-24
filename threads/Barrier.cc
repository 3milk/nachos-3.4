#include "Barrier.h"
#include "system.h"

Barrier* barrier;
Semaphore* blockParentBar;

Barrier::
Barrier(int threshold)
{
	mutex = new Lock("barrier_mutex");
	cond = new Condition("barrier_condition");
	nThreshold = threshold; 	// the number of thread to go to next round
	nThread = 0;	// the number of thread waiting on cond
	round = 0;
}

Barrier::
~Barrier()
{
	delete mutex;
	delete cond;
}

void
Barrier::barrier()
{
	mutex->Acquire();
	nThread++;
	if(nThread == nThreshold)
	{
		nThread = 0;
		round++;
		cond->Broadcast(mutex);
		printf("barrier pass - round %d\n", round-1);
		mutex->Release();
	} else {
		printf("barrier wait - round %d thread %d\n", round, currentThread->getTid());
		cond->Wait(mutex);
		mutex->Release();
	}
}

void InitBarrier(int threshold)
{
	barrier = new Barrier(threshold);
	blockParentBar = new Semaphore("blockParentBar", 0);
}

void RoundThread(int round)
{
	int i;

	for (i = 0; i < round; i++) {
		int t = barrier->GetRound();
	    ASSERT (i == t);
	    barrier->barrier();
	    printf("thread: %d, round: %d\n", currentThread->getTid(), i);
	}
	blockParentBar->V();
}

void DeleBarrier()
{
	delete barrier;
	delete blockParentBar;
}
