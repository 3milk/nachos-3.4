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
#include "uprogUtility.h"

#define MIN_FILE_SIZE 0//64
#define MAX_FILENAME_LEN 100

#define TransferSize 100
#define MAX_PATH_LEN 256

static void FetchStrFromUserSpace(char* buf, int size, int addr);
static void FetchFromUserSpace(char* buf, int size, int addr);
static void WriteBackToUserSpace(char* buf, int size, int addr);


static void SysCallPrintHandler();
static void SysCallPrintIntHandler();
static void SysCallExitHandler();

static void SysCallCreateHandler();
static void SysCallOpenHandler();
static void SysCallCloseHandler();
static void SysCallReadHandler();
static int SysCallWriteHandler();

static void SysCallExecHandler();
static void SysCallForkHandler();
static void SysCallYieldHandler();
static void SysCallJoinHandler();


// get the file name
//	do
//	{
//		machine->ReadMem(msg + i, 1, (int*)&buf[i]);
//	} while (++i<size);//while(buf[i++] != '\0');
void FetchStrFromUserSpace(char* buf, int size, int addr)
{
	int i = 0;
	bool pageFault = false;

	// Get string from user space.
	do
	{
		if(pageFault) {
			i--;
			pageFault = false;
		}

		if(!machine->ReadMem(addr + i, 1, (int*)&buf[i])) {
			// page fault, read fail
			pageFault = true;
		}
	}while(buf[i++] != '\0' || pageFault);
}

// write mem back
//	for(int i = 0; i<len; i++) {
//		machine->WriteMem(arg + i, 1, (int)buffer[i]);
//	}
void WriteBackToUserSpace(char* buf, int size, int addr)
{
	bool pageFault = false;

	for(int i = 0; i<size || pageFault; i++)
	{
		if(pageFault) {
			i--;
			pageFault = false;
		}
		if(!machine->WriteMem(addr + i, 1, (int)buf[i])) {
			// page fault, write fail
			pageFault = true;
		}
	}
}

//	for(int i = 0; i<size; i++) {
//		machine->ReadMem(arg + i, 1, (int*)&buffer[i]);
//	}
void FetchFromUserSpace(char* buf, int size, int addr)
{
	bool pageFault = false;

	for(int i = 0; i<size || pageFault; i++)
	{
		if(pageFault) {
			i--;
			pageFault = false;
		}
		if(!machine->ReadMem(addr + i, 1, (int*)&buf[i])) {
			// page fault, read fail
			pageFault = true;
		}
	}
}



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
    	case SC_PrintInt:
    		SysCallPrintIntHandler();
    		break;
    	case SC_Create:
    		SysCallCreateHandler();
    		break;
    	case SC_Open:
    		SysCallOpenHandler();
    		break;
    	case SC_Close:
    		SysCallCloseHandler();
    		break;
    	case SC_Read:
    		SysCallReadHandler();
    		break;
    	case SC_Write:
    		SysCallWriteHandler();
    		break;
    	case SC_Exec:
    		SysCallExecHandler();
    		break;
    	case SC_Fork:
    		SysCallForkHandler();
    		break;
    	case SC_Yield:
    		SysCallYieldHandler();
    		break;
    	case SC_Join:
    		SysCallJoinHandler();
    		break;
    	default:
    		break;
    	}
    } else if (which == PageFaultException) { // from TLB or PageTable
    	int addr = machine->ReadRegister(BadVAddrReg);
#ifdef VM
    		machine->SwapPage(addr); 		// load page from disk or swap file
#endif
    		machine->TLBSwap(addr);
	} else {
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}

static void SysCallExitHandler()
{
    int exitStatus = machine->ReadRegister(4);
    printf("SYSCALL: exit code %d, current Thread %s\n", exitStatus, currentThread->getName());

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
	int size = machine->ReadRegister(5);

	char* buf = new char[size + 5];
	memset(buf, 1, size+5);

	FetchStrFromUserSpace(buf, size+5, msg);
	printf("%s\n", buf);

	delete buf;

	machine->PCForward();
}

static void SysCallPrintIntHandler()
{
	int number = machine->ReadRegister(4);
	printf("%d\n", number);
	machine->PCForward();
}

static void SysCallCreateHandler()
{
	char fname[MAX_FILENAME_LEN];
	int arg = machine->ReadRegister(4);

	FetchStrFromUserSpace(fname, MAX_FILENAME_LEN, arg);

	if(fileSystem->Create(fname, MIN_FILE_SIZE)) {
		printf("SYSCALL: create file %s success\n", fname);
	} else {
		printf("SYSCALL: create file %s fail\n", fname);
	}

	machine->PCForward();
}

static void SysCallOpenHandler()
{
	int fid = -1;
	char fname[MAX_FILENAME_LEN];
	int arg = machine->ReadRegister(4);

	// Get the executable file name from user space.
	FetchStrFromUserSpace(fname, MAX_FILENAME_LEN, arg);

	fid = (int)fileSystem->Open(fname); // OpenFile pointer??? [x]fileheader sector?

	// return OpenFileId
	machine->WriteRegister(2, fid);
	machine->PCForward();
}

static void SysCallCloseHandler()
{
	OpenFileId fid = machine->ReadRegister(4);
	OpenFile* fp = (OpenFile*)fid;

	fp->~OpenFile();
	printf("SYSCALL: close file success\n");

	machine->PCForward();
}

static void SysCallReadHandler()
{
	int size = machine->ReadRegister(5);
	OpenFileId fid = machine->ReadRegister(6);
	OpenFile* fp = (OpenFile*)fid;
	int position = machine->ReadRegister(7);

	int len = 0;

	// check length
	len = fp->Length() > size? size: fp->Length();

	// read from the beginning of the file
	char* buffer = new char[len + 5];
	len = fp->Read(buffer, len, position);
	// copy from buffer to user space
	int arg = machine->ReadRegister(4);
	WriteBackToUserSpace(buffer, size, arg);

	delete [] buffer;
	//return int;
	machine->WriteRegister(2, len);
	machine->PCForward();
	printf("SYSCALL: read file %d byte\n", len);
}

static int SysCallWriteHandler()
{
	int size = machine->ReadRegister(5);
	OpenFileId fid = machine->ReadRegister(6);
	OpenFile* fp = (OpenFile*)fid;
	int position = machine->ReadRegister(7);

	char* buffer = new char[size + 5]; // must larger than size?!
	int arg = machine->ReadRegister(4);
	int len = 0;
	// Get the buffer content from user space.
	FetchFromUserSpace(buffer, size, arg);

	len = fp->Write(buffer, size, position);
	delete []buffer;

	//return int;
	machine->WriteRegister(2, len);
	machine->PCForward();
	printf("SYSCALL: write file %d byte\n", len);
}

#ifdef USER_PROGRAM

void ThreadFuncForUserProg(int arg)
{
	machine->Run();
}


static void SysCallExecHandler()
{
	//char* name = machine->ReadRegister(4);
	char fname[MAX_FILENAME_LEN];
	int arg = machine->ReadRegister(4);

	// Get the executable file name from user space.
	FetchStrFromUserSpace(fname, MAX_FILENAME_LEN, arg);

	// copy executable file from Unix to Nachos
	CopyExecFile(fname , fname);

    // Open the executable file.
	OpenFile* executable = fileSystem->Open(fname);
	if (executable != NULL)
	{
        // Set up a new thread and alloc address space for it.
		Thread* userThread = Thread::getInstance("exec user thread");
		AddrSpace* space = new AddrSpace(executable); //?? TODO use default noff file,
					//which offH.code.size, noffH.initData.size, noffH.uninitData.size are all 0
		space->AllocAddrSpace(userThread->getTid());
		userThread->space = space;

		userThread->InitUserState();

        // Return the new thread id.
		machine->WriteRegister(2, userThread->getTid());

		printf("SYSCALL: exec: %d %s\n", userThread->getTid(), userThread->getName());
		userThread->Fork(ThreadFuncForUserProg, 1);
	}
	else
	{
        // Can't open executable file, so return -1.
		machine->WriteRegister(2, -1);
	}

	machine->PCForward();
}

static void SysCallForkHandler()
{
	int ufunc = machine->ReadRegister(4);
	Thread* userThread = Thread::getInstance("fork user thread");
	AddrSpace* space = new AddrSpace(NULL); //?? TODO use default noff file,
			//which offH.code.size, noffH.initData.size, noffH.uninitData.size are all 0
	space->AllocAddrSpace(userThread->getTid());
	userThread->space = space;

	// Copy machine registers of current thread to new thread
	userThread->SaveUserState();
	// Modify PC/SP register of new thread
	userThread->WriteRegister(PCReg, ufunc);
	userThread->WriteRegister(NextPCReg, ufunc+4);
	userThread->WriteRegister(StackReg, space->GetNumPages()* PageSize - 16); //?? TODO

	printf("SYSCALL: fork: %d %s\n", userThread->getTid(), userThread->getName());
	//userThread->Fork(ThreadFuncForUserProg, 0);
	userThread->Fork(ThreadFuncForUserProg, 0);

	machine->PCForward();
}

static void SysCallYieldHandler()
{
    printf("SYSCALL: yield current thread: %d %s \n", currentThread->getTid(), currentThread->getName());
	currentThread->Yield();
	machine->PCForward();
}

static void SysCallJoinHandler()
{
	Thread *childThread;
	int exitStatus = 0;
	SpaceId childId = machine->ReadRegister(4);

	printf("SYSCALL: join: childID: %d\n", childId);

    // Check the waiting child thread whether in the exited child list or not.
    childThread = currentThread->removeExitedChild(childId);
    while (childThread == NULL)
    {
        // If the child thread is not in the exited child list, current thread sleep.
    	IntStatus oldLevel =  interrupt->SetLevel(IntOff); //??
        currentThread->Sleep(true); // no thread wake up this?
        childThread = currentThread->removeExitedChild(childId);
        (void) interrupt->SetLevel(oldLevel);
    }

    // Get child thread's exit status.
    exitStatus = childThread->getExitStatus();
    printf("SYSCALL: join: %d %s: exitStatus: %d\n", childThread->getTid(), childThread->getName(), exitStatus);

    delete childThread;

    // Return the child thread's exit status.
    machine->WriteRegister(2, exitStatus);
	machine->PCForward();
}
#endif
