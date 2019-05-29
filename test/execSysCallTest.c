/* execSysCallTest.c
 *    Test program to check progress system call in nachos
 *    Exec system call test program
 */

#include "syscall.h"

int
main()
{
	int arr[16];
	int i;
	Print("User: Exec Program\n", sizeof("User: Exec Program\n"));
	for(i = 0; i<16; i++) {
		PrintInt(i);
		if(i == 7)
			Yield();
	}
	Exit(0);
}
