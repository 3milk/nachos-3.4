// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "filehdr.h"
#include "filesys.h"


// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        BitMap *freeMap = new BitMap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        mapHdr->setCreateTime();
        mapHdr->setAccessTime();
        mapHdr->setUpdateTime();
        dirHdr->setCreateTime();
        dirHdr->setAccessTime();
        dirHdr->setUpdateTime();
        DEBUG('f', "Writing headers back to disk.\n");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
     
        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        // initialize root dir(/)
        directory->Initialize(DirectorySector, "/");

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile);	 // flush changes to disk
        directory->WriteBack(directoryFile);

        if (DebugIsEnabled('f')) {
        	freeMap->Print();
        	directory->Print();

        	delete freeMap;
        	delete directory;
        	delete mapHdr;
        	delete dirHdr;
        }
    } else {
    // if we are not formatting the disk, just open the files representing
    // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(char *name, int initialSize, int fileType)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize); // name: e.g. file

    directory = new Directory(NumDirEntries);
    // based on name(absolute), find the fileHdr-sector of the dirPath
    int dirPathSector = getDirPathSector(name, 1, 0);
    if(dirPathSector < 0)
    {	// no such path in directory
    	delete directory;
    	return FALSE;
    }

    // open dirPath file
    OpenFile* dirPathFile = new OpenFile(dirPathSector);
    directory->FetchFrom(dirPathFile);//(directoryFile);

    if (directory->Find(name) != -1) // search dir file
      success = FALSE;			// file is already in directory
    else {	
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();	// find a sector to hold the file header
    	if (sector == -1) 		
            success = FALSE;		// no free block for file header 
        else if (!directory->Add(name, sector, fileType))
            success = FALSE;	// no space in directory
        else {
    	    hdr = new FileHeader;
    	    if(fileType == FILETYPE_DIR)
    	    	initialSize = DirectoryFileSize;

    	    if (!hdr->Allocate(freeMap, initialSize))
            	success = FALSE;	// no space on disk for data
    	    else {
    	    	success = TRUE;
    	    	hdr->setCreateTime();
    	    	hdr->setAccessTime();
    	    	hdr->setUpdateTime();
    	    	hdr->WriteBack(sector);

    	    	if(fileType == FILETYPE_DIR)
    	    	{
    	    		// initialize directory file
    	    	    Directory* newDirectory = new Directory(NumDirEntries);
    	    	    OpenFile* newDirFile = new OpenFile(sector);  	// directory file header
    	    	    newDirectory->Initialize(sector, name);// hdr->getDataSector(0)  directory file 1st block
    	    	    newDirectory->WriteBack(newDirFile);
    	    	    delete newDirectory;
    	    	    delete newDirFile;
    	    	}
    	    	// everthing worked, flush all changes back to disk
    	    	directory->WriteBack(dirPathFile);
    	    	freeMap->WriteBack(freeMapFile);
    	    }
            delete hdr;
        }
        delete freeMap;
    }
    delete directory;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{ 
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "Opening file %s\n", name);

    int dirPathSector = getDirPathSector(name, 1, 0);
    if(dirPathSector < 0)
    {	// no such path in directory
    	printf("FileSystem::Open: no such path in directory\n");
      	delete directory;
       	return openFile;
    }
    OpenFile* dirPathFile = new OpenFile(dirPathSector);
    directory->FetchFrom(dirPathFile);
    //directory->FetchFrom(directoryFile);
    sector = directory->Find(name); 
    if (sector >= 0) 		
    	openFile = new OpenFile(sector);	// name was found in directory
    delete directory;				// 1cb0 --> 1c00
    delete dirPathFile;
    return openFile;				// return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(char *name)
{ 
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;
    int fileType;
    
    directory = new Directory(NumDirEntries);
    int dirPathSector = getDirPathSector(name, 1, 0);
    if(dirPathSector < 0)
    {	// no such path in directory
       	printf("FileSystem::Open: no such path in directory\n");
       	delete directory;
       	return FALSE;
    }
    OpenFile* dirPathFile = new OpenFile(dirPathSector);
    directory->FetchFrom(dirPathFile);


    //directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector == -1) {
       delete directory;
       delete dirPathFile;
       return FALSE;			 // file not found 
    }
    fileType = directory->getFileType(name);
//    fileHdr = new FileHeader;
//    fileHdr->FetchFrom(sector);
//
//    freeMap = new BitMap(NumSectors);
//    freeMap->FetchFrom(freeMapFile);

    if(fileType == FILETYPE_FILE) {
    	fileHdr = new FileHeader;
    	fileHdr->FetchFrom(sector);

    	freeMap = new BitMap(NumSectors);
    	freeMap->FetchFrom(freeMapFile);

    	fileHdr->Deallocate(freeMap);  		// remove data blocks
    	freeMap->Clear(sector);			// remove header block
    	directory->Remove(name);

    	freeMap->WriteBack(freeMapFile);		// flush to disk
    	directory->WriteBack(dirPathFile);        // flush to disk
    	delete fileHdr;
    	delete directory;
    	delete dirPathFile;
    	delete freeMap;
    	printf("FileSystem::Remove: file %s\n", name);
	} else if (fileType == FILETYPE_DIR) {
		Directory* dirDel = new Directory(NumDirEntries);
		OpenFile* dirDelFile = new OpenFile(sector);
		dirDel->FetchFrom(dirDelFile);
		int dirEntryNum = dirDel->getTableSize();
		for(int i = 1; i< dirEntryNum; i++) // 0 is current path, not remove
		{
			char* subFile = dirDel->getFileName(i);
			if(subFile != NULL)
				Remove(subFile);
		}

		fileHdr = new FileHeader;
    	fileHdr->FetchFrom(sector);

    	freeMap = new BitMap(NumSectors);
    	freeMap->FetchFrom(freeMapFile);

    	fileHdr->Deallocate(freeMap);  		// remove data blocks
    	freeMap->Clear(sector);			// remove header block
    	directory->Remove(name);

    	freeMap->WriteBack(freeMapFile);		// flush to disk
    	directory->WriteBack(dirPathFile);        // flush to disk

		delete dirDelFile;
		delete dirDel;
		delete fileHdr;
		delete directory;
		delete dirPathFile;
		delete freeMap;
		printf("FileSystem::Remove: dir %s and its sub files\n", name);
	}
//	freeMap->Clear(sector);			// remove header block
//    directory->Remove(name);
//
//    freeMap->WriteBack(freeMapFile);		// flush to disk
//    directory->WriteBack(directoryFile);        // flush to disk
//    delete fileHdr;
//    delete directory;
//    delete dirPathFile;
//    delete freeMap;
    return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Root Directory header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    printf("Bit map file:\n");
    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    printf("Root Directory:\n");
    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
} 

int
FileSystem::getDirPathSector(char* name, int level, int totalLevel, int dirFileSector)
{
	int res = -1;
	if(name == NULL)
		return res;

	if(level == 1)
	{
		// judge root path
		int totalLvl = 0;
		for(int i = 0; i < strlen(name); i++)
			if(name[i] == '/')
				++totalLvl;
		if(level == totalLvl)
		{ // /file.txt
			return DirectorySector;
		} else {
			//return getDirPathSector((char*)(name+1), level+1, totalLvl, DirectorySector); // cut, for relative path
			return getDirPathSector((char*)(name), level+1, totalLvl, DirectorySector); 	// no cut, for absolute path
		}
	} else {
		// /A/B/file.txt
		int start = 1, len = 0;
		for(int i = 0; i < strlen(name) && start <= level; i++)
		{
			if(name[i] == '/')
				++start;
			++len;
		}
		len--; // /A/ ---> /A
// for relative path
//		for(int i = 0; i < strlen(name); i++)
//		{
//			if(name[i] != '/')
//				++len;
//			else
//				break;
//		}
//
//		char* dirEntryStr = new char[len+1];
//		memcpy(dirEntryStr, name+1, len);
//		dirEntryStr[len] = '\0';
		char* dirEntryStr = new char[len+1];
		memcpy(dirEntryStr, name, len);
		dirEntryStr[len] = '\0';

		OpenFile* dirFile = new OpenFile(dirFileSector);
		Directory* directory = new Directory(NumDirEntries);
		directory->FetchFrom(dirFile);
		int sector = directory->Find(dirEntryStr);
		if(sector == -1){
			res = -1;
		} else if(level == totalLevel) {
			res = sector;
		} else {
			//res = getDirPathSector((char*)(name+len+1), level+1, totalLevel, sector);		// cut, for relative path
			res = getDirPathSector((char*)(name), level+1, totalLevel, sector);		// no cut, for absolute path
		}

		delete dirEntryStr;
		delete dirFile;
		delete directory;
	}
	return res;
}
