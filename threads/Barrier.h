#include "synch.h"

class Barrier
{
public:
	Barrier(int threshold);
	~Barrier();

	int GetRound() { return round; }// need mutex or not?
	void barrier();
private:
	Lock* mutex;
	Condition* cond;
	int nThreshold; 	// the number of thread to go to next round
	int nThread;	// the number of thread waiting on cond
	int round;		// round num
};


extern Barrier* barrier;
extern Semaphore* blockParentBar;
void InitBarrier(int threshold);
void RoundThread(int round);
void DeleBarrier();
