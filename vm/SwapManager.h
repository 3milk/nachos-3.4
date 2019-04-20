#ifndef SWAPPINGMANAGER_H
#define SWAPPINGMANAGER_H

#include "bitmap.h"

#define SWAP_SPACE_NAME	"Swapping"
#define MAX_SWAP_SPACE 	4096

class SwapManager
{
	public:
		SwapManager();
		~SwapManager();

		int swapIntoDisk(int physicalPage);
		void swapOutFromDisk(int physicalPage, int swappingPage);

		void Clear(int which);

	private:
		BitMap* swappingSpaceMap;
		OpenFile* swappingFile;
};

#endif
