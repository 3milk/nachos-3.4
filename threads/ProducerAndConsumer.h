// producer & consumer


#include "copyright.h"
#include "synch.h"

#define BUF_SIZE 8
extern bool buf[BUF_SIZE]; // true means full, false means empty
extern Semaphore* blockParent;

// implement by Semaphore
extern Semaphore* empty;
extern Semaphore* full;
extern Lock* bufMutex;

// implement by condition
extern Condition* emptyCon;
extern Condition* fullCon;
extern int emptyCnt;
extern Lock* conLock;

// implement by Semaphore
void InitPandC();
void DelePandC();
void Producer(int times);
void Consumer(int times);

// implement by condition
void InitPandC_Condition();
void DelePandC_Condition();
void Producer_Condition(int times);
void Consumer_Condition(int times);
