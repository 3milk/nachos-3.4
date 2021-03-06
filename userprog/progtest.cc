// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"
#include "uprogUtility.h"


/*
 * Funtion:	start to run userProgram instructions
 * */
void
ForkUserThread(int n)
{
	printf("ForkUserThread: %d %s\n", currentThread->getTid(), currentThread->getName());
	machine->Run();
}


//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
#ifdef FILESYS
#ifndef FILESYS_STUB
	// copy execfile from UNIX file to Nachos file
	// filename : '/Testfile'
	CopyExecFile(filename, filename);
#endif //FILESYS_STUB
#endif //FILESYS

    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }

    // create thread for user program
    Thread* userThread = Thread::getInstance(filename);
    // alloc addrSpace for userThread
    space = new AddrSpace(executable);
    space->AllocAddrSpace(userThread->getTid());
    userThread->space = space;
    // currentThread->space = space; // origin
    // init userThread user register (after alloc addrSpace)
    userThread->InitUserState();
    userThread->Fork(ForkUserThread, 50);

    // delete executable;			// close file when addrSpace delete

    // space->InitRegisters();		// origin // set the initial register values
    // space->RestoreState();		// origin // load page table register

    // machine->Run();			// origin // jump to the user progam
    // ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

//static Console *console;
//static Semaphore *readAvail;
//static Semaphore *writeDone;
static SynchConsole * synchConsole;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

//static void ReadAvail(int arg) { readAvail->V(); }
//static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    synchConsole = new SynchConsole(in, out);
    //console = new Console(in, out, ReadAvail, WriteDone, 0);
    //readAvail = new Semaphore("read avail", 0);
    //writeDone = new Semaphore("write done", 0);
    
    for (;;) {
    	//readAvail->P();		// wait for character to arrive
    	ch = synchConsole->GetChar();//console->GetChar();
    	synchConsole->PutChar(ch);//console->PutChar(ch);	// echo it!
    	//writeDone->P() ;        // wait for write to finish
    	if (ch == 'q') return;  // if q, quit
    }
}
