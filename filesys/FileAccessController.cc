#include "copyright.h"
#include "synch.h"
#include "system.h"
#include "FileAccessController.h"

FileAccessController::FileAccessController()
{

}


FileAccessController::~FileAccessController()
{

}


void
FileAccessController::open(int hdr)
{
	// assume that open a file, not a directory
	ASSERT(hdr >= 0 && hdr < NumSectors);
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	++facbs[hdr].referenceNum;
	if(facbs[hdr].rwlock == NULL) {
		facbs[hdr].rwlock = new RWLock();
	}
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

void
FileAccessController::close(int hdr)
{
	// assume that open a file, not a directory
	ASSERT(hdr >= 0 && hdr < NumSectors);
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	++facbs[hdr].referenceNum;
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

// call by FileSystem::Remove
bool
FileAccessController::remove(int hdr)
{
	bool canRemove = false;
	ASSERT(hdr >= 0 && hdr < NumSectors);
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	facbs[hdr].toRemove = true;
	if(facbs[hdr].referenceNum == 0)
	{
		canRemove = true;
	}
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
	return canRemove;
}

// call by OpenFile::~OpenFile()
bool
FileAccessController::checkRemove(int hdr)
{
	bool canRemove = false;
	ASSERT(hdr >= 0 && hdr < NumSectors);
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	if(facbs[hdr].referenceNum == 0 && facbs[hdr].toRemove)
	{
		canRemove = true;
	}
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
	return canRemove;
}

void
FileAccessController::finishRemove(int hdr)
{
	ASSERT(hdr >= 0 && hdr < NumSectors);
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	facbs[hdr].toRemove = false;
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}


bool
FileAccessController::getToRemove(int hdr)
{
	ASSERT(hdr >= 0 && hdr < NumSectors);
	return facbs[hdr].toRemove;
}

void
FileAccessController::rlock(int hdr)
{
	ASSERT(hdr >= 0 && hdr < NumSectors);
	//IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	//++facbs[hdr].referenceNum;
	facbs[hdr].rwlock->rlock();

	//(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}


void
FileAccessController::runlock(int hdr)
{
	ASSERT(hdr >= 0 && hdr < NumSectors);
	//IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	//--facbs[hdr].referenceNum;
	facbs[hdr].rwlock->runlock();

	//(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}


void
FileAccessController::wlock(int hdr)
{
	ASSERT(hdr >= 0 && hdr < NumSectors);
	//IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	//++facbs[hdr].referenceNum;
	facbs[hdr].rwlock->wlock();

	//(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}


void
FileAccessController::wunlock(int hdr)
{
	ASSERT(hdr >= 0 && hdr < NumSectors);
	//IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	//--facbs[hdr].referenceNum;
	facbs[hdr].rwlock->wunlock();

	//(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
