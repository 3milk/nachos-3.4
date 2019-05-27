// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif
#include <string.h>

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}


bool
AddrSpace::AllocAddrSpace(int tid)
{
//	if(execFile == NULL)
//		return false;

	if(tid < 0 || tid >= MAX_THREADS_NUM)
		return false;
	threadId = tid;

	NoffHeader noffH;
	unsigned int i, size;
	bool shareFromParent = false;

	if (execFile == NULL) {
		// syscall - fork: copy parent memoey and execFile
		// getParent AddrSpace( copy execFile & read page table content)
		Thread* thread = tid_pointer[tid];
		Thread* parent = thread->getParent();
		AddrSpace* parAddr = parent->getAddrSpace();
		// execFile copy, size = parent size
		numPages = parAddr->GetNumPages();
		execFile = parAddr->getExecFileCopy();
		shareFromParent = true;
	} else {
		// execFile != NULL
		execFile->ReadAt((char *)&noffH, sizeof(noffH), 0);
		if ((noffH.noffMagic != NOFFMAGIC) &&
			(WordToHost(noffH.noffMagic) == NOFFMAGIC))
		   	SwapHeader(&noffH);
		ASSERT(noffH.noffMagic == NOFFMAGIC);

		// how big is address space?
		size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
				+ UserStackSize;	// we need to increase the size
									// to leave room for the stack
		numPages = divRoundUp(size, PageSize);
	}
	size = numPages * PageSize;


	//ASSERT(numPages <= NumPhysPages);		// check we're not trying
							// to run anything too big --
							// at least until we have
							// virtual memory

	// check if we have enough empty physical memory
	int numEmptyPages = memManager->NumEmpty();
	int allocedPages = numPages;
	if(numPages > numEmptyPages) {
		DEBUG('a', "No enough physical memory, request %d, fact %d\n",
								numPages, numEmptyPages);
		// return NULL;
		printf("AllocAddrSpace: No enough physical memory, request %d, fact %d\n",
								numPages, numEmptyPages);
		allocedPages = numEmptyPages;
	}

	DEBUG('a', "Initializing address space, num pages %d, size %d\n",
						numPages, size);

	// if use lazy-load strategy, set allocedPages 0, then all PTE.valid will be false
	if(machine->UseLazyLoad())
		allocedPages = 0;

	// first, set up the translation
	int physicalAddr = 0;
	pageTable = new TranslationEntry[numPages];
	for (i = 0; i < allocedPages; i++) {//numPages; i++) {
		// valid Pages
		pageTable[i].virtualPage = i;	// for now, virtual page # = first empty phys page #
		pageTable[i].physicalPage = memManager->FindNext();
		memManager->SetPhyMemPage(pageTable[i].physicalPage, threadId, i);
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
						// a separate page, we could set its
						// pages to be read-only
		// zero out each page in address space, to zero the unitialized data segment
		// and the stack segment
		pageTable[i].lastUseTime = stats->totalTicks;
		memManager->UpdateLastUsedTime(pageTable[i].physicalPage, pageTable[i].lastUseTime);
		pageTable[i].swappingPage = -1;
		physicalAddr = pageTable[i].physicalPage * PageSize;
		bzero(machine->mainMemory + physicalAddr, PageSize);
	}
	for ( ; i < numPages; i++) {
		// invalid Pages
		pageTable[i].virtualPage = i;
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = FALSE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;
		pageTable[i].lastUseTime = 0;
		pageTable[i].swappingPage = -1;
	}


	if(shareFromParent) {
		// syscall - fork: code is not in execFile, code is in host memory
		// no need to copy from host memory to Nachos main memory
		Thread* thread = tid_pointer[tid];
		Thread* parent = thread->getParent();
		AddrSpace* parAddr = parent->getAddrSpace();
		return CopyMemFromParent(parAddr);
	}
	// if (execFile != NULL)
	// then, copy in the code and data segments into memory
	// byte by byte
	int  bufSize =  noffH.code.size > noffH.initData.size ? noffH.code.size : noffH.initData.size;
	char *buffer = new char[bufSize];

	DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
		    		   noffH.code.virtualAddr, noffH.code.size);
	int va, vpn, offset, pa;
	int cnt = 0, allocedSize = allocedPages * PageSize;	// just load alloced size in main memory, because code+initData size might be larger than alloced size
	execFile->ReadAt(buffer, noffH.code.size, noffH.code.inFileAddr);
	for(i = 0; i < noffH.code.size && cnt < allocedSize; i++, cnt++)
	{
		va = noffH.code.virtualAddr + i;
		vpn = va / PageSize;
		offset = va % PageSize;
		pa = pageTable[vpn].physicalPage * PageSize + offset;
		machine->mainMemory[pa] = buffer[i];
	}

	DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
		   		noffH.initData.virtualAddr, noffH.initData.size);
	execFile->ReadAt(buffer, noffH.initData.size, noffH.initData.inFileAddr);
	for(i = 0; i < noffH.initData.size && cnt < allocedSize; i++, cnt++)
	{
		va = noffH.initData.virtualAddr + i;
		vpn = va / PageSize;
		offset = va % PageSize;
		pa = pageTable[vpn].physicalPage * PageSize + offset;
		machine->mainMemory[pa] = buffer[i];
	}

	delete[] buffer;
	return true;
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
	execFile = executable;

	/*
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = i;
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
*/
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
   delete execFile;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	// For now, nothing!
//  Update phyMemPageTable based on current thread addrspace->pageTable
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
	int vpn = 0;
	int phyNum = 0;
	int lastUsedTime = 0;
	// update phyMemPageTable by pageTable
	for(int i = 0; i<numPages; i++) {
		phyNum = pageTable[i].physicalPage;
		lastUsedTime = pageTable[i].lastUseTime;
		memManager->UpdateLastUsedTime(phyNum, lastUsedTime);
	}
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

int AddrSpace::GetNumPages()
{
	return numPages;
}

int
AddrSpace::LazyLoad(int phyPageNum, int vpn)
{
	NoffHeader noffH;
	int startAddr = vpn * PageSize;
	int phyPosition = phyPageNum * PageSize;

	execFile->ReadAt((char *)&noffH, sizeof(noffH), 0);
	// virtual mem: code + initData + uninitData + UserStackSize
	if (startAddr < noffH.code.size) {
		if(startAddr + PageSize < noffH.code.size)
		{
			// load from code
			execFile->ReadAt(&machine->mainMemory[phyPosition], PageSize, noffH.code.inFileAddr + startAddr);
			printf("LazyLoad: from code\n");
		} else {
			// load from code and initData
			int codeSize = noffH.code.size - startAddr;
			execFile->ReadAt(&machine->mainMemory[phyPosition], codeSize, noffH.code.inFileAddr + startAddr);
			int initDataSize = startAddr + PageSize - noffH.code.size;
			execFile->ReadAt(&machine->mainMemory[phyPosition + codeSize], initDataSize, noffH.initData.inFileAddr);
			printf("LazyLoad: from code and initData\n");
		}
	} else if(startAddr < noffH.code.size + noffH.initData.size)
	{
		// load from initData
		startAddr -= noffH.code.size;
		int size = (startAddr + PageSize > noffH.initData.size)
				? (startAddr + PageSize - noffH.initData.size)
					: PageSize ;
		execFile->ReadAt(&machine->mainMemory[phyPosition], size, noffH.initData.inFileAddr + startAddr);
		printf("LazyLoad: from initData\n");
		if(size < PageSize)
		{
			// part load from uninitData, which equals to 0
			bzero(machine->mainMemory + (phyPosition + size), PageSize - size);
			printf("LazyLoad: from uninitData or UserStack\n");
		}
	} else {
		// load from uninitData or userstack, which equals to 0
		bzero(machine->mainMemory + phyPosition, PageSize);
		printf("LazyLoad: from uninitData or UserStack\n");
	}


	return 0;
}

int
AddrSpace::getPTESwappingPage(int vpn)
{
	if(vpn < 0 || vpn > numPages)
		return -1;
	return pageTable[vpn].swappingPage;
}

void
AddrSpace::setPTESwappingPage(int vpn, int swappingPage)
{
	if(vpn < 0 || vpn > numPages)
		return ;
	pageTable[vpn].swappingPage = swappingPage;
}

bool
AddrSpace::getPTEValid(int vpn)
{
	if(vpn < 0 || vpn > numPages)
		return -1;
	return pageTable[vpn].valid;
}

void
AddrSpace::setPTEValid(int vpn, bool value)
{
	if(vpn < 0 || vpn > numPages)
		return ;
	pageTable[vpn].valid = value;
}

int
AddrSpace::getPTEPPN(int vpn)
{
	if(vpn < 0 || vpn > numPages)
		return -1;
	return pageTable[vpn].physicalPage;
}

bool
AddrSpace::CopyMemFromParent(AddrSpace* parAddr)
{
	for(int i = 0; i<numPages; i++)
	{
		int childPhyPage = pageTable[i].physicalPage;
		int parPhyPage = parAddr->getPTEPPN(i);
		if(pageTable[i].valid)
		{
			// child PTE is valid
			if(parAddr->getPTEValid(i)) {
				// parent page is in MainMemory
				memcpy(&machine->mainMemory[childPhyPage], &machine->mainMemory[parPhyPage], PageSize);
			} else if(parAddr->getPTESwappingPage(i) == -1) {
				// parent page is in execFile
				LazyLoad(childPhyPage, i);
			} else {
				// parent page is in Swapping
				int swappingPage = parAddr->getPTESwappingPage(i);
				swapManager->swapOutFromDisk(childPhyPage, swappingPage, true);
			}
		} else { // !pageTable[i].valid
			// child PTE is invalid
			if(parAddr->getPTEValid(i)) {
				// parent page is in MainMemory
				int swappingPage = swapManager->swapIntoDisk(parPhyPage);
				pageTable[i].swappingPage = swappingPage;
			} else if(parAddr->getPTESwappingPage(i) == -1) {
				// parent page is in execFile
				// do nothing
			} else {
				// parent page is in Swapping
				int swappingPage = parAddr->getPTESwappingPage(i);
				pageTable[i].swappingPage = swapManager->copySwapping(swappingPage);
			}
		}
	}
	return true;
}
