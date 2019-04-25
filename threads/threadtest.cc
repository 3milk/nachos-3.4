// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "test_hello.h"
#include "ProducerAndConsumer.h"

// testnum is set in main.cc
int testnum = 7;//1;


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num = 0;
    
    for (num = 0; num < 3; num++) {
    	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = Thread::getInstance("forked thread");
    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

void
SimpleThread2(int which)
{
	printf("*** thread %d running.\n", which);
	currentThread->Yield();
}

//----------------------------------------------------------------------
// ThreadTest2
// 	test the up limit of total threads in Nachos
//----------------------------------------------------------------------

void
ThreadTest2()
{
	DEBUG('t', "Entering ThreadTest1");

	for(int i = 0; i<130; i++){
	   	Thread *t = Thread::getInstance("forked thread");
	   	if(t != NULL){
	   		t->Fork(SimpleThread2, t->getTid());
	   	}
	}
	SimpleThread2(0);
	currentThread->Yield();

	for(int i = 0; i<3; i++){
		Thread *t = Thread::getInstance("forked thread");
	   	if(t != NULL){
	   		t->Fork(SimpleThread2, t->getTid());
		}
	}
	Thread::TS();
	SimpleThread2(0);
}

//----------------------------------------------------------------------
// ThreadTest3
// priority scheduler
//----------------------------------------------------------------------

void
ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");

    /*Thread *t1 = Thread::getInstance("forked thread", HIGH);
    Thread *t2 = Thread::getInstance("forked thread", MIDDLE);
    Thread *t3 = Thread::getInstance("forked thread", LOW);
    //t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    t1->Fork(SimpleThread, 1);
    t3->Fork(SimpleThread, 3);
*/
    Thread *t1 = Thread::getInstance("forked thread", MIDDLE);
    Thread *t2 = Thread::getInstance("forked thread", MIDDLE);
    Thread *t3 = Thread::getInstance("forked thread", HIGH);
    t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    //t1->Fork(SimpleThread, 1);
    t3->Fork(SimpleThread, 3);

    //SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest4
// priority scheduler
// yield when process
//----------------------------------------------------------------------
void p3(int which)
{
	int i;
	for(i = 0; i<3; i++)
	{
	  	printf("*** thread %d priority %d looped %d times\n", which, currentThread->getPriority(), i);
	  	currentThread->Yield();
	}
}

void p2(int which)
{
	int i;
	for(i = 0; i<3; i++)
	{
	  	printf("*** thread %d priority %d looped %d times\n", which, currentThread->getPriority(), i);
	  	currentThread->Yield();
	  	if(i == 0)
	  	{
	  	  	Thread *t3 = Thread::getInstance("forked thread", LOW);
	  	  	t3->Fork(p3, 3);
	  	}
	}
}

void p1(int which)
{
	int i;
	for(i = 0; i<3; i++)
	{
	  	printf("*** thread %d priority %d looped %d times\n", which, currentThread->getPriority(), i);
	  	currentThread->Yield();
	  	if(i == 0)
	  	{
	  	   	Thread *t2 = Thread::getInstance("forked thread", HIGH);
	  	   	t2->Fork(p2, 2);
	  	}
	}
}

void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");
    Thread *t1 = Thread::getInstance("forked thread", MIDDLE);
    t1->Fork(p1, 1);

    /*Thread *t1 = Thread::getInstance("forked thread", HIGH);
    Thread *t2 = Thread::getInstance("forked thread", MIDDLE);
    Thread *t3 = Thread::getInstance("forked thread", LOW);
    //t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    t1->Fork(SimpleThread, 1);
    t3->Fork(SimpleThread, 3);
*/
    /*
    Thread *t1 = Thread::getInstance("forked thread", MIDDLE);
    Thread *t2 = Thread::getInstance("forked thread", MIDDLE);
    Thread *t3 = Thread::getInstance("forked thread", HIGH);
    t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    //t1->Fork(SimpleThread, 1);
    t3->Fork(SimpleThread, 3);
*/
}

//----------------------------------------------------------------------
// ThreadTest5
// 	RR scheduler
//----------------------------------------------------------------------
void
SimpleThread5(int n)
{

    for (int i = 1; i <= n; ++i) {
        printf("***thread tid: %d, looped %d times, pri: %d\n", currentThread->getTid(), i, currentThread->getPriority());
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
}


void
ThreadTest5()
{
	Thread * t1 = Thread::getInstance("t1");
    Thread * t2 = Thread::getInstance("t2");
    Thread * t3 = Thread::getInstance("t3");

    t1->Fork(SimpleThread5, 50);
    t2->Fork(SimpleThread5, 50);
    t3->Fork(SimpleThread5, 50);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------


void ProducerAndConsumerTest()
{
	int cNum = 2;
	InitPandC();
	printf("ProducerAndConsumerTest start\n");
	Thread* p1 = Thread::getInstance("p1");
	Thread* c1 = Thread::getInstance("c1");
	Thread* c2 = Thread::getInstance("c2");
	p1->Fork(Producer, 20);
	c1->Fork(Consumer, 10);
	c2->Fork(Consumer, 10);
	printf("ProducerAndConsumerTest wait\n");
	for(int i = 0; i<cNum; i++)
		blockParent->P(); // wait consumers thread finish
	printf("ProducerAndConsumerTest finish\n");
	DelePandC();
}

void ProducerAndConsumerTestCondition()
{
	int cNum = 2;
	InitPandC_Condition();
	printf("ProducerAndConsumerTest_Condition start\n");
	Thread* p1 = Thread::getInstance("p1");
	Thread* c1 = Thread::getInstance("c1");
	Thread* c2 = Thread::getInstance("c2");
	p1->Fork(Producer_Condition, 20);
	c1->Fork(Consumer_Condition, 10);
	c2->Fork(Consumer_Condition, 10);
	printf("ProducerAndConsumerTest_Condition wait\n");
	for(int i = 0; i<cNum; i++)
		blockParent->P(); // wait consumers thread finish
	printf("ProducerAndConsumerTest_Condition finish\n");
	DelePandC_Condition();
}

void
ThreadTest()
{
    switch (testnum) {
    case 1:
    	ThreadTest1();
    	break;
    case 2:
    	ThreadTest2();
    	break;
    case 3:
    	ThreadTest3();
        break;
    case 4:
    	ThreadTest4();
    	break;
    case 5:
    	ThreadTest5();
    	break;
    case 6: // producer and consumer
    	ProducerAndConsumerTest();
    	break;
    case 7:
    	ProducerAndConsumerTestCondition();
    	break;
    default:
	printf("No test specified.\n");
	break;
    }
}

