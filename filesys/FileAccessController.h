// FileAccessController.h
//	1. synchronize readers and writers when they request to access the same file
//  2. delete the file if and only if all reference proocesses finish
//  TODO not implement remove the file/derectory atomically

#ifndef FAC_H
#define FAC_H

#include "copyright.h"
#include "utility.h"
#include "synch.h"
#include "RWLock.h"

//#define FILESYS // TODO add to Makefile, not here
#ifdef FILESYS
struct FileAccCtrlBlock {
	int referenceNum;
	RWLock* rwlock;
	bool toRemove;	// set TRUE when File::Remove was called (consider derectory remove?!)
		// check this variable when some file close (~OpenFile())
	FileAccCtrlBlock(): referenceNum(0), rwlock(NULL), toRemove(false) {}
	~FileAccCtrlBlock()
	{
		if(rwlock != NULL)
			delete rwlock;
	}
};

class FileAccessController {
public:
	FileAccessController();
	~FileAccessController();

	void open(int hdr);
	void close(int hdr);
	bool remove(int hdr);		// called by FileSystem::Remove
	bool checkRemove(int hdr);	// called by OpenFile::~OpenFile
	void finishRemove(int hdr);
	bool getToRemove(int hdr);	// called by FileSystem::Open
	void rlock(int hdr);
	void runlock(int hdr);
	void wlock(int hdr);
	void wunlock(int hdr);
private:
	FileAccCtrlBlock facbs[NumSectors];
};


#endif //FILESYS

#endif //FAC

