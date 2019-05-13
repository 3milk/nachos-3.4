// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"


#define DateLen 24 // Wed Feb 13 15:46:11 2013

//#define NumDirect 	((SectorSize - 2 * sizeof(int) - 3 * (DateLen + 1)) / sizeof(int))
//#define MaxFileSize 	(NumDirect * SectorSize)

#define NumDirAndIndir ((SectorSize - 2 * sizeof(int) - 3 * (DateLen + 1)) / sizeof(int))
#define NumDirect 6
#define NumIndirect (NumDirAndIndir - NumDirect) // 5
#define NumIndex ((SectorSize - sizeof(int)) / sizeof(short)) // 62
#define MaxBlockNum (NumDirect + NumIndirect * NumIndex) // 316
#define MaxFileSize (MaxBlockNum * SectorSize) // 40448B = 39.5M

#define DATA_SECTOR_UNUSED -1 // in dataSectors, when item is not used, it will be set as -1

// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader {
  public:
	FileHeader(){}
	~FileHeader(){}
    bool Allocate(BitMap *bitMap, int fileSize);// Initialize a file header, 
						//  including allocating space 
						//  on disk for the file data
    bool ExtendAllocate(BitMap *freeMap, int fileSize);
    void Deallocate(BitMap *bitMap);  		// De-allocate this file's 
						//  data blocks

    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk

    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte

    int FileLength();			// Return the length of the file 
					// in bytes

    void Print();			// Print the contents of the file.
    void setCreateTime();
    void setUpdateTime();
    void setAccessTime();

  private:
    int numBytes;			// Number of bytes in the file
    int numSectors;			// Number of data sectors in the file
    int dataSectors[NumDirAndIndir];//[NumDirect];		// Disk sector numbers for each data
					// block in the file
    char createTime[DateLen+1];		// create time
    char updateTime[DateLen+1];		// last update time
    char accessTime[DateLen+1];		// last access time

    char* getCurrentTime();
};


/*
 * one FileIndexTable uses one sector
 * because an index is represented in short type,
 * there are total 128/2 = 64 indexes in a table
 * */
class FileIndexTable
{
	public:
	FileIndexTable();
	~FileIndexTable();

	bool Allocate(int sector);
	void FetchFrom(int sector);
	void WriteBack(int sector);		// Write modifications to file index file

	int getDataSector(int idx) { ASSERT(idx >=0 && idx < NumIndex); return (int)indexTable[idx]; }
	int getNumIndexes() { return numIndexes; }
	private:
	int numIndexes;				// Number of indexes in the file
	short indexTable[NumIndex]; // Disk sector numbers for each data
								// block in the file
};

#endif // FILEHDR_H
