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
SwapManager::swapOutFromDisk(int physicalPage, int swappingPage)
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

		swappingSpaceMap->Clear(swappingPage);
	}
	else
	{
		printf("Swap Out Failed!\n");
	}
}

void
SwapManager::Clear(int which)
{
	swappingSpaceMap->Clear(which);
}
