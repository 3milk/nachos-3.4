// fstest.cc 
//	Simple test routines for the file system.  
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file 
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"
#include "directory.h"
#include "synch.h"

#define TransferSize 	10 	// make it small, just to be difficult


//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void
Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(from, "r")) == NULL) {	 
	printf("Copy: couldn't open input file %s\n", from);
	return;
    }

// Figure out length of UNIX file
    fseek(fp, 0, 2);		
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

// Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength)) {	 // Create Nachos file
    	printf("Copy: couldn't create output file %s\n", to);
    	fclose(fp);
    	return;
    }
    
    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);
    
// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
    	openFile->Write(buffer, amountRead);
    delete [] buffer;

// Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *name)
{
    OpenFile *openFile;    
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
    }
    
    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
	for (i = 0; i < amountRead; i++)
	    printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;		// close the Nachos file
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"/TestFile"
//#define FileName 	"/A/B/TestFile"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
#define FileSize 	((int)(ContentSize ))//* 4000))//5000))

#define DirNameA	"/A"
#define DirNameB	"/A/B"
#define	FileNameErr	"/B/TestFile"

#define W_BUF_SIZE 2*SectorSize
#define R_BUF_SIZE 10*W_BUF_SIZE
#define readerYield 	1

enum FStestCase {
	FSTEST_FIRST_INDEX_FILE,
	FSTEST_MULTI_LEVEL_DIRECTORY,
	FSTEST_EXTEND_FILE_SET,
	FSTEST_EXTEND_FILE_WRITE,
	FSTEST_MULTI_THREADS_READWRITE,
	FSTEST_REMOVE
};

static void
CreateDir(char* dirName)
{
	OpenFile *openFile;
	int i, numBytes;

	printf("Create file directory %s\n", dirName);
	if (!fileSystem->Create(dirName, 0, FILETYPE_DIR)) { // extend FileSize dynamically
		printf("Perf test: can't create %s\n", dirName);
	    return;
	}
}

static void
RemoveDir(char* dirName)
{
	if (!fileSystem->Remove(dirName)) {
	    printf("Perf test: unable to remove %s\n", dirName);
	    return;
	}
}

static void 
FileWrite()
{
    OpenFile *openFile;    
    int i, numBytes;

    printf("Sequential write of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);
    if (!fileSystem->Create(FileName, FileSize)) {// 0)) { // extend FileSize dynamically
      printf("Perf test: can't create %s\n", FileName);
      return;
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
    	printf("Perf test: unable to open %s\n", FileName);
    	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
        if (numBytes < 10) {
        	printf("Perf test: unable to write %s\n", FileName);
        	delete openFile;
        	return;
		}
    }
    delete openFile;	// close file
}

static void 
FileRead(int tmp = 0)
{
    OpenFile *openFile;    
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("Sequential read of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);

    if ((openFile = fileSystem->Open(FileName)) == NULL) {
    	printf("Perf test: unable to open file %s\n", FileName);
    	delete [] buffer;
    	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Read(buffer, ContentSize);
        if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
        	printf("Perf test: unable to read %s\n", FileName);
        	delete openFile;
        	delete [] buffer;
        	return;
        }
    }
    delete [] buffer;
    delete openFile;	// close file
}

// need  #define FileName 	"/TestFile"
void TestFirstIndexFile()
{
	printf("[PerformanceTest] Starting file system performance test:\n");
	stats->Print();
	FileWrite();
	printf("[PerformanceTest] After write\n");
	fileSystem->List();
	fileSystem->Print();
	FileRead();
    if (!fileSystem->Remove(FileName)) {
    	printf("Perf test: unable to remove %s\n", FileName);
	    return;
	}
	printf("[PerformanceTest] Finish\n");
	stats->Print();
}

// need  #define FileName 	"/A/B/TestFile"
//		 #define DirNameA	"/A"
//		 #define DirNameB	"/A/B"
void TestMultiLevelDirectory()
{
	char* dirA = DirNameA;
	char* dirB = DirNameB;
	printf("[PerformanceTest] Starting file system performance test:\n");
	stats->Print();
	CreateDir(dirA);
	CreateDir(dirB);
	FileWrite();
	printf("[PerformanceTest] After write\n");
	fileSystem->List();
	fileSystem->Print();
	FileRead();
	RemoveDir(dirA);
	printf("[PerformanceTest] Finish\n");
	stats->Print();
}


// orgSize: the size when file create
// extendSize: the size to be extendSize
// e.g. orgSize 10, extendSize 20, finalSize 10 + 20 = 30
void TestExtendFileSet(char* name, int orgSize, int extendSize)
{
	printf("\n\n\n\nPerf: Create file %s \n", name);
	if(!fileSystem->Create(name, orgSize)) {
		printf("Perf test: can't create %s\n", FileName);
		return;
	}
	fileSystem->Print();
	printf("\n\n\n\nPerf: Extend file size from %d to %d\n", orgSize, orgSize + extendSize);
	if(!fileSystem->ExtendFile(name, extendSize)) {
		printf("Perf test: extend file size fail: file: %s origSize: %d extendSize: %d\n", name, orgSize, extendSize);
		return;
	}
	fileSystem->Print();
	printf("Perf finish~\n");
}


// need  #define FileName 	"/TestFile"
// in FileWrite, create init size is 0
void TestExtendFileWrite()
{
	printf("[PerformanceTest] Starting file system performance test:\n");
	stats->Print();
	FileWrite(); // TODO
	printf("[PerformanceTest] After write\n");
	fileSystem->List();
	fileSystem->Print();
    if (!fileSystem->Remove(FileName)) {
    	printf("Perf test: unable to remove %s\n", FileName);
	    return;
	}
	printf("[PerformanceTest] Finish\n");
	stats->Print();
}

Semaphore* writerFinish = NULL;
void FileWriter(int start)
{
	OpenFile* file = fileSystem->Open(FileName);
	if(file == NULL) {
		printf("FileWriter:: open fail: %s\n", FileName);
		return;
	}
	char ch = '0';
	char buf[W_BUF_SIZE];
	for(int i = start; i<start+3; i++) {
		ch = '0' + i;
		memset(buf, ch, W_BUF_SIZE);
		file->Write(buf, sizeof(buf), SEEK_POS_END);
		printf("%s: file length: %d\n", currentThread->getName(), file->Length());
		currentThread->Yield();
	}
	delete file;
	writerFinish->V();
}


void FileReader(int cnt)
{
	OpenFile* file = fileSystem->Open(FileName);
	if(file == NULL) {
		printf("FileReader:: open fail: %s\n", FileName);
		return;
	}
	char buf[R_BUF_SIZE];
	for(int i = 0; i<cnt; i++) {
		int len = file->Length();
		if(len >= R_BUF_SIZE) {
			printf("Reader:: larger than buf size\n");
			break;
		}
		int successLen = file->Read(buf, len, SEEK_POS_SET);
		buf[successLen] = '\0';
		printf("%s: file length: %d\n\tfile content %s\n", currentThread->getName(), successLen, buf);
#ifdef FSTESTREMOVE
if(readerYield)
#endif
		currentThread->Yield();
	}
	delete file;
}

// create file /Testfile
// w1: write 0,1,
// w2: write 5,6
void TestMultiThreadsReadWrite()
{
	printf("TestMultiThreadsReadWrite\n");
	// create file
	if (!fileSystem->Create(FileName, 0)) {
		printf("Perf test: can't create %s\n", FileName);
	    return;
	}

	writerFinish = new Semaphore("writerFinish", 0);
	// create reader and writer thread
	Thread* r1 = Thread::getInstance("Reader 1");
	Thread* r2 = Thread::getInstance("Reader 2");
	Thread* r3 = Thread::getInstance("Reader 3");
	Thread* w1 = Thread::getInstance("Writer 1");
	Thread* w2 = Thread::getInstance("Writer 2");
	int readerRound = 3;
	w1->Fork(FileWriter, 1);
	r1->Fork(FileReader, readerRound);
	r2->Fork(FileReader, readerRound);
	w2->Fork(FileWriter, 5);
	for(int i = 0; i < 2; i++)
		writerFinish->P();
	r3->Fork(FileReader, 1);
	delete writerFinish;
}


void FileRemove(int tmp)
{
	printf("FileRemove\n");
	fileSystem->Remove(FileName);
}

// change the file size
// test case 1: FIFO: r1(read one time, no yield) --- rm
// no reference then remove directly
// test case 2: FIFO: r1(read 2 times, yield) --- rm
// r1 still ref, rm when r1 close the file
void TestRemove()
{
	// create file and write 12345678
	FileWrite();
	// create 2 readers and 1 remover thread
	Thread* r1 = Thread::getInstance("r1");
	Thread* rm = Thread::getInstance("rm");

	r1->Fork(FileReader, 1); //2
	rm->Fork(FileRemove, 0);
}

void
PerformanceTest()
{
	int testCase = FSTEST_REMOVE;//FSTEST_MULTI_THREADS_READWRITE;//FSTEST_EXTEND_FILE_SET;
	switch(testCase) {
	case FSTEST_FIRST_INDEX_FILE:
		TestFirstIndexFile();
		break;
	case FSTEST_MULTI_LEVEL_DIRECTORY:
		TestMultiLevelDirectory();
		break;
	case FSTEST_EXTEND_FILE_SET:
		{
			char* fileName = "/FileEx";
			int fileSize = 200; // (NumDirect + NumIndex) * SectorSize  // (NumDirect + NumIndex + 1) * SectorSize
			int extendSize = 400; // 600 // NumIndex * SectorSize
			TestExtendFileSet(fileName, fileSize, extendSize);
			break;
		}
	case FSTEST_EXTEND_FILE_WRITE:
		TestExtendFileWrite();
		break;
	case FSTEST_MULTI_THREADS_READWRITE:
		TestMultiThreadsReadWrite(); // set MakeFile -DFSTEST_MULTI_THREADS_READWRITE
		break;
	case FSTEST_REMOVE: // // set MakeFile -DFSTESTREMOVE
		TestRemove();
		break;
	}
}

