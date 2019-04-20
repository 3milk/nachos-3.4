#include "syscall.h"
#define ARR_SIZE 1024
int arr[ARR_SIZE];

int
main()
{
	int i;
	// init arr
	for (i = 0; i < ARR_SIZE; i++) {
	    arr[i] = i;
	    Print(i, sizeof(int));
	}
	//Halt();
    Exit(0);		/* and then we're done -- should be 0! */
}
