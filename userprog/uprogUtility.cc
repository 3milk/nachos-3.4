#include "uprogUtility.h"

#define TransferSize 100
#define MAX_PATH_LEN 256

//----------------------------------------------------------------------
// CopyExecFile
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void
CopyExecFile(char *from, char *to)
{
// current path
	char realPath[MAX_PATH_LEN];
	getcwd(realPath, 128);
	strcat(realPath, "/test");
	strcat(realPath, from);

	FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(realPath, "r")) == NULL) {
	printf("Copy: couldn't open input file %s\n", realPath);
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
