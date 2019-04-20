/* multi_proc_1.c
 *    Test program to check multi-process in nachos
 */

#include "syscall.h"
#define ARR_SIZE 16//64
int arr[ARR_SIZE];

int
main()
{
	int i;
	for (i = 0; i < ARR_SIZE; i++) {
	    Print(2, sizeof(int));
	    Print(i, sizeof(int));
	}

	return 0;
	//Halt();
    //Exit(0);		/* and then we're done -- should be 0! */
}
