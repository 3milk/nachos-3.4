// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"


static void SysCallPrintHandler();
static void SysCallExitHandler();

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException)) {
    	switch(type)
    	{
    	case SC_Halt:
    		DEBUG('a', "Shutdown, initiated by user program.\n");
    		interrupt->Halt();
    		break;
    	case SC_Exit:
    		SysCallExitHandler();
    		break;
    	case SC_Print:
    		DEBUG('a', "Print, call by user program.\n");
    		SysCallPrintHandler();
    		break;
    	default:
    		break;
    	}
    } else if (which == PageFaultException) { // from TLB or PageTable
    	int addr = machine->ReadRegister(BadVAddrReg);
    	// if(machine->TLBSwap(addr) < 0)
    	//{	// PageFaultException from PageTable
#ifdef VM
    		machine->SwapPage(addr); 		// load page from disk or swap file
#endif
    		machine->TLBSwap(addr);
    	//}
	} else {
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}

static void SysCallExitHandler()
{
    int exitStatus = machine->ReadRegister(4);

    // Delete thread' address space.
	currentThread->DeleteAddrSpace();

    // Set thread's exit status.
    currentThread->setExitStatus(exitStatus);

    // Thread finished.
	currentThread->Finish();
}

static void SysCallPrintHandler()
{
	int msg = machine->ReadRegister(4);
	printf("%d\n", msg);
	/*
	int size = machine->ReadRegister(5);
	//printf("%d\n",msg);

	char* buf = new char[size + 5];

	int i = 0;
	do
	{
		machine->ReadMem(msg + i, 1, (int*)&buf[i]);
	}while(++i<size);//(buf[i++] != '\0');

	printf("%s\n", buf);

	delete buf;
*/
	machine->PCForward();
}
