// MemManager.h

#ifndef MEMMANAGER_H
#define MEMMANAGER_H

#include "copyright.h"
#include "utility.h"
#include "bitmap.h"
#include "addrspace.h"

struct PhyMemPageEntry {
	int threadId;		// thread id
	int virtualPage;	// virtual page number
	int lastUsedTime;	// for swap in/out
	PhyMemPageEntry(): threadId(-1),virtualPage(-1), lastUsedTime(0) {}
	void ZeroPhyMemPageEntry()
	{
		threadId = -1;
		virtualPage = -1;
		lastUsedTime = 0;
	}
};

class MemManager {
  public:
	MemManager(int nitems);		// Initialize a bitmap, with "nitems" bits
				// initially, all bits are cleared.
    ~MemManager();			// De-allocate bitmap

    void Mark(int which);   	// Set the "nth" bit
    void Clear(int which);  	// Clear the "nth" bit
    bool Test(int which);   	// Is the "nth" bit set?
    int FindNext();            	// Return the # of a clear bit (an empty frame),
    							// and as a side effect, set the bit.
								// If no bits are clear, return -1.
    int NumEmpty();				// Return the number of clear bits (empty frames)

    void Print();				// Print contents of bitmap

    // base on phyMemPageTable
    bool SetPhyMemPage(int phyNum, int tid, int virNum);
    bool UpdateLastUsedTime(int phyNum, int lut);
    int GetThreadId(int phyNum);
    int GetVirPageNum(int phyNum);

    int FindSwapPage(bool* unused);	// LRU

    bool ZeroPhyMemPage(int phyNum);

  private:
    int pageTableEntryNum;
    BitMap* bitmap;				// stats of main memory
    PhyMemPageEntry* phyMemPageTable;
};

#endif
