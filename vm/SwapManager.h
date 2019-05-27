#ifndef SWAPPINGMANAGER_H
#define SWAPPINGMANAGER_H

#include "bitmap.h"

#define SWAP_SPACE_NAME	"/Swapping"
#define MAX_SWAP_SPACE 	256//4096

class SwapManager
{
	public:
		SwapManager();
		~SwapManager();

		int swapIntoDisk(int physicalPage);
		void swapOutFromDisk(int physicalPage, int swappingPage, bool copy = false);
		int copySwapping(int copyPage);

		void Clear(int which);

	private:
		BitMap* swappingSpaceMap;
		OpenFile* swappingFile;
};

#endif
