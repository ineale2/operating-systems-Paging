/* vgetmem.c - vgetmem */

#include <xinu.h>

static void vmeminit(struct procent* prptr);

char  	*vgetmem(
	  uint32	nbytes		/* Size of memory requested	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	memblk	*prev, *curr, *leftover, *vmemlist;
	struct procent* prptr;
	mask = disable();
	if (nbytes == 0) {
		restore(mask);
		return (char *)SYSERR;
	}

	nbytes = (uint32) roundmb(nbytes);	/* Use memblk multiples	*/
	prptr = &proctab[currpid];
	// Return if this process does not have virtual memory enabled
	if( prptr->vh != VHEAP){
		restore(mask);
		return (char *)SYSERR;
	}
	if( prptr->vmeminit == 0){
		vmeminit(prptr);
	}
	vmemlist = &(prptr->vmemlist);

	
	prev = vmemlist;
	curr = vmemlist->mnext;
	while (curr != NULL) {			/* Search free list	*/

		if (curr->mlength == nbytes) {	/* Block is exact match	*/
			prev->mnext = curr->mnext;
			vmemlist->mlength -= nbytes;
			restore(mask);
			return (char *)(curr);

		} else if (curr->mlength > nbytes) { /* Split big block	*/
			leftover = (struct memblk *)((uint32) curr +
					nbytes);
			prev->mnext = leftover;
			leftover->mnext = curr->mnext;
			leftover->mlength = curr->mlength - nbytes;
			vmemlist->mlength -= nbytes;
			restore(mask);
			return (char *)(curr);
		} else {			/* Move to next block	*/
			prev = curr;
			curr = curr->mnext;
		}
	}
	restore(mask);

	return (char *)SYSERR;
}

static void vmeminit(struct procent* prptr){
	struct memblk *memptr;
	memptr = &prptr->vmemlist;

	/* Initialize the memory counter and head of free list */
	memptr->mlength = NBPG*prptr->hsize;
	memptr->mnext   =(struct memblk*)VHEAP_START;


	/* All vheap memory is free initially, one large block */
	memptr = memptr->mnext;
	memptr->mnext = NULL;
	memptr->mlength = NBPG*prptr->hsize;

	prptr->vmeminit = 1;
}
