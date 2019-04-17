/* srpolicy.c - srpolicy */

#include <xinu.h>

int32 currpolicy;

/*------------------------------------------------------------------------
 *  srplicy  -  Set the page replacement policy.
 *------------------------------------------------------------------------
 */
syscall srpolicy(int policy)
{
	switch (policy) {
	case FIFO:
		currpolicy = FIFO;
		return OK;

	case GCA:
		currpolicy = GCA;
		/* Initialize the GCA semaphore */
		if(SYSERR == gca_sem){
			kprintf("Could not initialize GCA semaphore\n");
			return SYSERR;
		}
		else
			return OK;

	default:
		return SYSERR;
	}
}
