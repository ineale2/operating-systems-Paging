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
		return OK;

	default:
		return SYSERR;
	}
}
