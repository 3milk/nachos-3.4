// MemManager.cc

#include "copyright.h"
#include "memmanager.h"

// Initialize a bitmap, with "nitems" bits
// initially, all bits are cleared.
MemManager::MemManager(int nitems)
{
	pageTableEntryNum = nitems;
	bitmap = new BitMap(nitems);
	phyMemPageTable = new PhyMemPageEntry[nitems];
}

// De-allocate bitmap
MemManager::~MemManager()
{
	delete bitmap;
	delete[] phyMemPageTable;
}

// Set the "nth" bit
void
MemManager::Mark(int which)
{
	return bitmap->Mark(which);
}

// Clear the "nth" bit
void
MemManager::Clear(int which)
{
	return bitmap->Clear(which);
}

// Is the "nth" bit set?
bool
MemManager::Test(int which)
{
	return bitmap->Test(which);
}

// Return the # of a clear bit (an empty frame),
// and as a side effect, set the bit.
// If no bits are clear, return -1.
int
MemManager::FindNext()
{
	return bitmap->Find();
}

// Return the number of clear bits (empty frames)
int
MemManager::NumEmpty()
{
	return bitmap->NumClear();
}

// Print contents of bitmap
void
MemManager::Print()
{
	return bitmap->Print();
}

bool
MemManager::ZeroPhyMemPage(int phyNum)
{
	if(phyNum > pageTableEntryNum)
		return false;
	phyMemPageTable[phyNum].ZeroPhyMemPageEntry();
	return true;
}

bool
MemManager::SetPhyMemPage(int phyNum, int tid, int virNum)
{
	if(phyNum > pageTableEntryNum)
		return false;
	phyMemPageTable[phyNum].threadId = tid;
	phyMemPageTable[phyNum].virtualPage = virNum;
	return true;
}

bool
MemManager::UpdateLastUsedTime(int phyNum, int lut)
{
	if(phyNum > pageTableEntryNum)
		return false;
	phyMemPageTable[phyNum].lastUsedTime = lut;
	return true;
}

int
MemManager::GetThreadId(int phyNum)
{
	if(phyNum < 0 || phyNum > pageTableEntryNum)
		return -1;
	return phyMemPageTable[phyNum].threadId;
}

int
MemManager::GetVirPageNum(int phyNum)
{
	if(phyNum < 0 || phyNum > pageTableEntryNum)
		return -1;
	return phyMemPageTable[phyNum].virtualPage;
}

/*
 * Function: 	find a physical page to swap from memory to "disk", based on LRU
 * return:		physical page number
 * */
int
MemManager::FindSwapPage(bool* unused)
{
	int idx = 0;
	int min = phyMemPageTable[0].lastUsedTime;
	for(int i = 0; i<pageTableEntryNum; i++)
	{
		if(phyMemPageTable[i].lastUsedTime < min)
		{
			idx = i;
			min = phyMemPageTable[i].lastUsedTime;
		}
	}

	*unused = !bitmap->Test(idx);

	return idx;
}
