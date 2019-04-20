/* tlb_test.c
 *    Test program to check TLB swap algorithm.
 *
 *    //Intention is to stress virtual memory system.
 *
 *
 */

#include "syscall.h"
#define ARR_SIZE 16//64
int arr[ARR_SIZE];

int add()
{
	int sum, i = 0;
	for(i = 0; i<ARR_SIZE; i++)
	{
		sum += arr[i];
	}
	return sum;
}

int 
main()
{
	int i;
	char str[8];
	for(i = 0; i<7; i++)
		str[i] = 'a'+i;
	str[8] = '\0';
	// init arr
	for (i = 0; i < ARR_SIZE; i++)
	        arr[i] = i;
	i = add();
    //printf("add: %d\n", add());


	Print(str, sizeof(str));
	Print(i, sizeof(int));
	Halt();
    //Exit(0);		/* and then we're done -- should be 0! */
}
