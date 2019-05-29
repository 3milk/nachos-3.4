/* progSysCallTest.c
 *    Test program to check progress call in nachos
 *    Fork, Exec, Yield, Exit, Join
 */

#include "syscall.h"

void ForkFunc()
{
	int i;
	Print("User: ForkFunc\n", sizeof("User: ForkFunc\n"));
	for(i = 0; i<16; i++) {
		PrintInt(i);
		if(i == 7)
			Yield();
	}
	Exit(0);
}

int
main()
{
	int execID;
	int exitStatus;
	Fork(ForkFunc);
	execID = Exec("/execSysCallTest");
	exitStatus = Join(execID);
	PrintInt(exitStatus);

	Exit(0);
}
