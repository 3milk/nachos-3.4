#include "system.h"
#include "machine.h"
#include "SwapManager.h"

/*
 * Function: 		create the swapping space bitmap
 * 					create a swapping file to simulate swapping space in the disk
 * 					open the swapping file
 * */
SwapManager::SwapManager()
{
	swappingSpaceMap = new BitMap(MAX_SWAP_SPACE);

	fileSystem->Create(SWAP_SPACE_NAME, MAX_SWAP_SPACE * PageSize);
	swappingFile = fileSystem->Open(SWAP_SPACE_NAME);
}

SwapManager::~SwapManager()
{
	delete swappingSpaceMap;
	fileSystem->Remove(SWAP_SPACE_NAME);
}

/*
 * Function: 		swap page from memory to "disk"
 * physicalPage: 	physical page number in memory
 * return:			if success, return the page number in "disk"
 * 					else, return -1
 * */
int
SwapManager::swapIntoDisk(int physicalPage)
{
	int swappingPage;
	int phyMemPosition;
	int swappingPosition;

	if (swappingSpaceMap->NumClear() != 0)
	{
		swappingPage = swappingSpaceMap->Find();
		phyMemPosition = physicalPage * PageSize;
		swappingPosition = swappingPage * PageSize;

		swappingFile->WriteAt(&(machine->mainMemory[phyMemPosition]),
								PageSize,
								swappingPosition);
	}
	else
	{
		printf("Swap In Failed!\n");
		swappingPage = -1;
	}

	return swappingPage;
}

/*
 * Function: 		swap page from "disk" to memory
 * physicalPage: 	physical page number in memory
 * swappingPage:	page number in "disk", this page is to be swapped into memory
 * */
void
SwapManager::swapOutFromDisk(int physicalPage, int swappingPage, bool copy)
{
	int phyMemPosition;
	int swappingPosition;

	if (swappingSpaceMap->Test(swappingPage))
	{
		phyMemPosition = physicalPage * PageSize;
		swappingPosition = swappingPage * PageSize;
		swappingFile->ReadAt(&(machine->mainMemory[phyMemPosition]),
							 PageSize,
							 swappingPosition);

		if(!copy) {
			// AddrSpace::CopyMemFromParent
			// just copy from Swapping for child phyPage
			// remain the Swapping for parent phyPage
			swappingSpaceMap->Clear(swappingPage);
		}
	}
	else
	{
		printf("Swap Out Failed!\n");
	}
}

int
SwapManager::copySwapping(int copyPage)
{
	int writePage;
	char buf[PageSize];
	int copyPosition; // orig copy
	int writePosition;// final copy save

	if (swappingSpaceMap->NumClear() != 0)
	{
		writePage = swappingSpaceMap->Find();
		copyPosition = copyPage * PageSize;
		writePosition = writePage * PageSize;

		swappingFile->ReadAt(buf, PageSize, copyPosition);
		swappingFile->WriteAt(buf, PageSize, writePosition);
	}
	else
	{
		printf("copy Swap Failed!\n");
		writePage = -1;
	}

	return writePage;
}

void
SwapManager::Clear(int which)
{
	swappingSpaceMap->Clear(which);
}
