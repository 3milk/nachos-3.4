// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "translate.h"

#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"

    bool AllocAddrSpace(int tid);	// alloc memory to a specific thread, which thread id is tid
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 

    int GetNumPages();
    int LazyLoad(int phyPageNum, int vpn);
    int getPTESwappingPage(int vpn);
    void setPTESwappingPage(int vpn, int swappingPage);
    bool getPTEValid(int vpn);
    void setPTEValid(int vpn, bool value);
    int getPTEPPN(int vpn);

    OpenFile* getExecFileCopy() { return execFile->GetFileDescriptorCopy();}
    bool CopyMemFromParent(AddrSpace* parADdr);
  private:
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
    OpenFile *execFile;
    int threadId;
};

#endif // ADDRSPACE_H
