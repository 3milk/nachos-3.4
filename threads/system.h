// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H
#define MAX_THREADS_NUM 128
#define USER_KERNEL -1

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"


// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock

// edit from here
extern int threadNum;				// threadNum
extern int tid_flag[MAX_THREADS_NUM]; // thread ID flag, 0 for empty, 1 for occupied
extern Thread *tid_pointer[MAX_THREADS_NUM]; // thread pointer, match tid to thread pointer

#ifdef USER_PROGRAM
#include "machine.h"
#include "memmanager.h"
extern Machine* machine;	// user program memory and registers
extern MemManager* memManager; // Memory Manager to alloc/dealloc physical memory
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "FileAccessController.h"
extern FileAccessController *fileAccessController;
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#ifdef VM
#include "SwapManager.h"
extern SwapManager* swapManager; // Swap Manager to
#endif

#endif // SYSTEM_H
