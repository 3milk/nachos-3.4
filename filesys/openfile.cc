// openfile.cc 
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "openfile.h"
#include "system.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector, int parSector)
{ 
    hdr = new FileHeader;
    hdrSector = sector;
    parHdrSector = parSector;
    hdr->FetchFrom(sector);

    //hdr->setAccessTime();
    hdr->WriteBack(sector);
    seekPosition = 0;

    fileAccessController->open(sector);
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
	printf("Close file: hdr %d\n", hdrSector);
    if(fileAccessController->checkRemove(hdrSector)
    		&& parHdrSector != -1)
    {
    	// find filename to remove
    	Directory *directory = new Directory(NumDirEntries);
    	OpenFile* dirPathFile = new OpenFile(parHdrSector);
    	directory->FetchFrom(dirPathFile); // not this, it supposed to be the parentDirPath file
    	char* name = directory->Find(hdrSector);
    	if(name!=NULL)
    		fileSystem->Remove(name);
    	else
    		printf("OpenFile::~OpenFile: filename is NULL\n");

    	delete dirPathFile;
    	delete directory;
    }
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void
OpenFile::Seek(int position)
{
    seekPosition = position;
}	

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes, int position)
{
	if(position < SEEK_POS_END)
		return -1;
	fileAccessController->rlock(hdrSector);
	hdr->FetchFrom(hdrSector);

	switch(position)
	{
	case SEEK_POS_SET:
		this->Seek(0);
		break;
	case SEEK_POS_CUR:
		break;
	case SEEK_POS_END:
		this->Seek(hdr->FileLength());
		break;
	default:
		this->Seek(position);
		break;
	}

	int result = ReadAt(into, numBytes, seekPosition, true);
	seekPosition += result;

	hdr->setAccessTime();
	hdr->WriteBack(hdrSector);

	fileAccessController->runlock(hdrSector);
	return result;
}

int
OpenFile::Write(char *into, int numBytes, int position)
{
	if(position < SEEK_POS_END)
		return -1;
	fileAccessController->wlock(hdrSector);
	hdr->FetchFrom(hdrSector);

	switch(position)
	{
		case SEEK_POS_SET:
			this->Seek(0);
			break;
		case SEEK_POS_CUR:
			break;
		case SEEK_POS_END:
			this->Seek(hdr->FileLength());
			break;
		default:
			this->Seek(position);
			break;
	}

	int result = WriteAt(into, numBytes, seekPosition, true);
	seekPosition += result;

	hdr->setAccessTime();
	hdr->setUpdateTime();
	hdr->WriteBack(hdrSector);

	fileAccessController->wunlock(hdrSector);
	return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position, bool locked)
{
	//if(!locked) // outside is not locked
	//	fileAccessController->rlock(hdrSector);

    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
    	return 0; 				// check request
    if ((position + numBytes) > fileLength)		
    	numBytes = fileLength - position;
    DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n", 	
		numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)	{
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
#ifdef FSTEST_MULTI_THREADS_READ_WRITE
    if(locked)
        currentThread->Yield();
#endif
    }
    // copy the part we want
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);

    //if(!locked) // outside is not locked
    //	fileAccessController->runlock(hdrSector);

    delete [] buf;
    return numBytes;
}

int
OpenFile::WriteAt(char *from, int numBytes, int position, bool locked)
{
	//if(!locked) // outside is not locked
	//	fileAccessController->wlock(hdrSector);

    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if ((numBytes <= 0)) {// || (position >= fileLength))
    	//fileAccessController->wunlock(hdrSector);
    	return 0;				// check request
    }
    if ((position + numBytes) > fileLength) {
    	if(fileSystem->ExtendFile(hdrSector, position + numBytes - fileLength)) {
    		// if extend file successfully, we should update hdr here
    		hdr->FetchFrom(hdrSector);
    		printf("OpenFile::WriteAt: extend file from %d to %d\n", fileLength, hdr->FileLength());
    		fileLength = hdr->FileLength();
    	} else if (position < fileLength) {
    		numBytes = fileLength - position;
    		printf("OpenFile::WriteAt: extend file fail. only %d bytes can be write\n", numBytes);
    	} else {
    		printf("OpenFile::WriteAt: file to write");
    		//fileAccessController->wunlock(hdrSector);
    		return 0;
    	}
    }

    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];

    firstAligned = (position == (firstSector * SectorSize));
    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

// read in first and last sector, if they are to be partially modified
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);	
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
				SectorSize, lastSector * SectorSize);	

// copy in the bytes we want to change 
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

// write modified sectors back
    for (i = firstSector; i <= lastSector; i++)	{
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
#ifdef FSTEST_MULTI_THREADS_READ_WRITE
    if(locked)
        currentThread->Yield();
#endif
    }

    //if(!locked) // outside is not locked
    //	fileAccessController->wunlock(hdrSector);

    delete [] buf;
    return numBytes;
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
OpenFile::Length() 
{ 
    return hdr->FileLength(); 
}


OpenFile*
OpenFile::GetFileDescriptorCopy()
{
	return new OpenFile(hdrSector, parHdrSector);
}
