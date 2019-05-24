/* fileSysCallTest.c
 *    Test program to check fileSystem call in nachos
 *    Create, Open, Read, Write, Close
 */

#include "syscall.h"
#define Filename "/TestFile"
#define BUF_SIZE 32

int
main()
{
	char rbuf[BUF_SIZE];
	char* wbuf = "1234567890";
	int wlen = 11;
	OpenFileId fid = 0;
	int len = 0;

	// create and open file
	Create(Filename);
	fid = Open(Filename);
	if(fid == 0) {
		Print("open file fail\n", sizeof("open file fail\n"));
	}

	// read (nothing in file)
	len = Read(rbuf, BUF_SIZE, fid, SEEK_POS_U_SET);
	Print("Read content:", sizeof("Read content:"));
	Print(rbuf, len+1);
	Print("\n", sizeof("\n"));

	len = Write(wbuf, wlen, fid, SEEK_POS_U_CUR);
	Print("Write Len:", sizeof("Write Len:"));
	PrintInt(len);

	// read (something in file)
	len = Read(rbuf, BUF_SIZE, fid, SEEK_POS_U_SET);
	Print("Read content:", sizeof("Read content:"));
	Print(rbuf, len+1);
	Print("\n", sizeof("\n"));

	// close file
	Close(fid);

	Exit(0);
}
