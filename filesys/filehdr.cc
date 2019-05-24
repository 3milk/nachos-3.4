// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include <time.h>

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
	ASSERT(fileSize <= MaxFileSize);
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
    	return FALSE;		// not enough space

    // initialize as unused
    for (int i = 0; i < NumDirAndIndir; i++)
   		dataSectors[i] = DATA_SECTOR_UNUSED;

    // alloc used
    if(numSectors <= NumDirect) {
    	for (int i = 0; i < numSectors; i++)
    		dataSectors[i] = freeMap->Find();
    } else {
    	// how many index block do we need?
    	int idxBlockNum = divRoundUp((numSectors - NumDirect), NumIndex);
    	if(freeMap->NumClear() < numSectors + idxBlockNum)
    		return FALSE;	// not enough space

    	// alloc direct index
    	for (int i = 0; i < NumDirect; i++)
    		dataSectors[i] = freeMap->Find();

    	// alloc indirect index
    	int leftBlocks = numSectors - NumDirect;
    	for (int i = 0; i < idxBlockNum; i++, leftBlocks -= NumIndex)
    	{
    		FileIndexTable* idxTable = new FileIndexTable();
    		int sectorToSave = freeMap->Find();
    		dataSectors[NumDirect + i] = sectorToSave;

    		int numIndex = NumIndex;
    		if(leftBlocks < NumIndex)
    			numIndex = leftBlocks;
    		for (int j = 0; j < numIndex; j++)
    		{
    			idxTable->Allocate(freeMap->Find());
    		}

    		idxTable->WriteBack(sectorToSave);
    		delete idxTable;
    	}
    }
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------


bool
FileHeader::ExtendAllocate(BitMap *freeMap, int fileSize)
{
	// 1. calc original sector number
	//	  calc needed additional sector number
	//    can not calc seperately, because the last sector of orig alloc block may not be used totally,
	//    we need to merge new content with the last orig block
	int needByte = fileSize;
	//int needSector = divRoundUp(needByte, SectorSize);
	int originalByte = FileLength();
	int originalSector = divRoundUp(originalByte, SectorSize);
	bool lastOrigBlockFull = originalByte % SectorSize? true: false;
	int totalByte = needByte + originalByte;
	int totalSector = divRoundUp(totalByte, SectorSize);
	int needSector = totalSector - originalSector;
	if(freeMap->NumClear() < needSector)
		return FALSE;	// no enough space

	printf("alloc new sector for extend file: size: %d, length: %d, from last orig: %d\n", needByte, needSector, lastOrigBlockFull);
	if(totalByte > MaxFileSize)
	{
		printf("[FileHeader::ExtendAllocate] ERR	large than MaxFileSize.\n");
		return FALSE;
	}

	if(originalSector <= NumDirect) {
		// 2. last orig is direct index
		//    1) new end is direct index (if success, update numBytes, numSectors)
		//    2) new end is indirect index (if success, update numBytes, numSectors)

		// TODO dataSector needing init outside? or here? (use FetchFrom)
		if(totalSector <= NumDirect) {
			// 1) new end is direct index
			for(int i = 0; i<needSector; i++) {
				dataSectors[originalSector + i] = freeMap->Find();
			}
			numBytes = totalByte;
			numSectors = totalSector;
		} else {
			// 2) new end is indirect index
			// how many index block do we need?
			int idxBlockNum = divRoundUp(totalSector-NumDirect, NumIndex);
			if((idxBlockNum + needSector) > freeMap->NumClear()) {
				printf("[FileHeader::ExtendAllocate] ERR	 no enough space.\n");
				return FALSE;
			}
			// alloc direct
			for(int i = originalSector; i<NumDirect; i++) {
				dataSectors[i] = freeMap->Find();
			}
			// alloc indirect
			int leftBlocks = needSector - (NumDirect - originalSector);
			for (int i = 0; i < idxBlockNum; i++, leftBlocks -= NumIndex)
			{
				FileIndexTable* idxTable = new FileIndexTable();
				int sectorToSave = freeMap->Find();
				dataSectors[NumDirect + i] = sectorToSave;

				int numIndex = NumIndex;
			    if(leftBlocks < NumIndex)
			    	numIndex = leftBlocks;
			    for (int j = 0; j < numIndex; j++)
			    {
			    	idxTable->Allocate(freeMap->Find());
			    }

			    idxTable->WriteBack(sectorToSave);
			    delete idxTable;
			}
			numBytes = totalByte;
			numSectors = totalSector;
		}
	} else {
		// 3. last orig is indirect index (if success, update numBytes, numSectors)
		//    1) last index reference block is full
		//    2) last index reference block is not full

		int origIdxBlockNum = divRoundUp(originalSector-NumDirect, NumIndex);
		int origBlockUsedInLastIdx = (originalSector-NumDirect) % NumIndex;
		int origBlockUnusedInLastIdx = NumIndex - origBlockUsedInLastIdx;
		// how many index block do we need?
		int idxBlockNum = 0;
		if(needSector-origBlockUnusedInLastIdx > 0)
			idxBlockNum = divRoundUp(needSector-origBlockUnusedInLastIdx, NumIndex);
		if((idxBlockNum + needSector) > freeMap->NumClear()) {
			printf("[FileHeader::ExtendAllocate] ERR	 no enough space.\n");
			return FALSE;
		}

		// fill unused block in last index
		int lastIdxBlock = dataSectors[NumDirect + origIdxBlockNum - 1];
		FileIndexTable* idxTableLast = new FileIndexTable();
		idxTableLast->FetchFrom(lastIdxBlock);
		int needSectorInLatIdx = needSector > origBlockUnusedInLastIdx ?
				origBlockUnusedInLastIdx : needSector;
		for(int i = 0; i < needSectorInLatIdx; i++)
		{
			idxTableLast->Allocate(freeMap->Find());
		}
		idxTableLast->WriteBack(lastIdxBlock);
		delete idxTableLast;

		// alloc new idx and blocks
		int leftBlocks = needSector > origBlockUnusedInLastIdx ?
				needSector - origBlockUnusedInLastIdx : 0;
		for (int i = 0; i < idxBlockNum; i++, leftBlocks -= NumIndex)
		{
			FileIndexTable* idxTable = new FileIndexTable();
			int sectorToSave = freeMap->Find();
			dataSectors[origIdxBlockNum + i] = sectorToSave;

			int numIndex = NumIndex;
			if(leftBlocks < NumIndex)
			  	numIndex = leftBlocks;
			for (int j = 0; j < numIndex; j++)
			{
			   	idxTable->Allocate(freeMap->Find());
			}

			idxTable->WriteBack(sectorToSave);
			delete idxTable;
		}
		numBytes = totalByte;
		numSectors = totalSector;
	}
}


void 
FileHeader::Deallocate(BitMap *freeMap)
{
//    for (int i = 0; i < numSectors; i++) {
//    	ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
//		freeMap->Clear((int) dataSectors[i]);
//    }

	// deallocate direct
    for (int i = 0; i < NumDirect; i++) {
    	if(dataSectors[i] == DATA_SECTOR_UNUSED)
    		return;
        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
    	freeMap->Clear((int) dataSectors[i]);
    }

    // deallocate indirect
    FileIndexTable *idxTable = new FileIndexTable();
    for (int i = NumDirect; i < NumDirAndIndir; i++) {
    	if(dataSectors[i] == DATA_SECTOR_UNUSED)
    	    break;
    	ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!

    	int idxSector = (int)dataSectors[i];
    	idxTable->FetchFrom(idxSector);
    	int numIndexes = idxTable->getNumIndexes();
    	for (int j = 0; j < numIndexes; j++) {
    		ASSERT(freeMap->Test((int)idxTable->getDataSector(j)));  // ought to be marked!
    		freeMap->Clear((int)idxTable->getDataSector(j));
    	}

    	freeMap->Clear(idxSector);
    }
    delete idxTable;
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    //return(dataSectors[offset / SectorSize]);

	int blockNum = offset / SectorSize;
	if(blockNum < NumDirect) {
		return (dataSectors[offset / SectorSize]);
	} else {
		int idx = (blockNum - NumDirect) / NumIndex;
		int idxSector = dataSectors[idx + NumDirect];
		FileIndexTable* idxTable = new FileIndexTable();
		idxTable->FetchFrom(idxSector);
		int idxTableOffset = (blockNum - NumDirect) % NumIndex;
		int sector = (int)(idxTable->getDataSector(idxTableOffset));
		delete idxTable;
		return sector;
	}
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d  ", numBytes);
    printf("createTime: %s. updateTime: %s.  accessTime: %s  \nFile blocks: ", createTime, updateTime, accessTime);
    for (i = 0; i < NumDirAndIndir; i++)
    	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < NumDirect; i++){ //numSectors; i++) {
    	if(dataSectors[i] == DATA_SECTOR_UNUSED)
    		break;
    	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
        	if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
        		printf("%c", data[j]);
            else
            	printf("\\%x", (unsigned char)data[j]);
        }
        printf("\n"); 
    }
    // TODO idxFileTable and indirect data block
    delete [] data;
}


void
FileHeader::setCreateTime()
{
	strncpy(createTime, getCurrentTime(), DateLen);
}

void
FileHeader::setUpdateTime()
{
	strncpy(updateTime, getCurrentTime(), DateLen);
}

void
FileHeader::setAccessTime()
{
	strncpy(accessTime, getCurrentTime(), DateLen);
}

char*
FileHeader::getCurrentTime()
{
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	return asctime(timeinfo);
}


FileIndexTable::FileIndexTable()
{
	numIndexes = 0;
}

FileIndexTable::~FileIndexTable()
{}

// find an empty sector based on bitmap
// save the sector number into dataSector
// dataSector create fileIndexTable
// fileIndexTable alloc index for sector and save
bool
FileIndexTable::Allocate(int sector)
{
	if (numIndexes >= NumIndex)
		return FALSE;		// not enough space, use other file index table

	// suppose blocks are placed sequentially
	indexTable[numIndexes++] = sector;
	return TRUE;
}
//----------------------------------------------------------------------
// FileIndexTable::FetchFrom
// 	Fetch contents of file index table from disk.
//
//	"sector" is the disk sector containing the file index table
//----------------------------------------------------------------------

void
FileIndexTable::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileIndexTable::WriteBack
// 	Write the modified contents of the file index table back to disk.
//
//	"sector" is the disk sector to contain the file index tablelastIdxBlock
//----------------------------------------------------------------------

void
FileIndexTable::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this);
}
