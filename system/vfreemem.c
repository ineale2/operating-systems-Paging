/* vfreemem.c - vfreemem */

#include <xinu.h>

syscall	vfreemem(
	  char		*blkaddr,	/* Pointer to memory block	*/
	  uint32	nbytes		/* Size of block in bytes	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	memblk	*next, *prev, *block, *vmemlist;
	struct  procent *prptr;
	uint32	top;

	mask = disable();
	debug("vfreemem: currpid = %d, blkaddr = 0x%08x, nbytes = %d == 0x%08x\n", currpid, blkaddr, nbytes, nbytes);
	prptr = &proctab[currpid];
	char* vheap_end = (char*)VHEAP_START + NBPG*prptr->hsize;
	if ((nbytes == 0) || ((uint32) blkaddr < VHEAP_START)
			  || (blkaddr >  vheap_end)) {
		restore(mask);
		debug("vfreemem: minheap = %08x, maxheap = %08x\n");
		return SYSERR;
	}
	// Return if this process does not have virtual memory enabled
	if( prptr->vh != VHEAP){
		restore(mask);
		debug("vfreemem: process has no virtual heap, prptr->vh = %d\n", prptr->vh);
		return SYSERR;
	}

	nbytes = (uint32) roundmb(nbytes);	/* Use memblk multiples	*/
	block = (struct memblk *)blkaddr;
	
	vmemlist = &prptr->vmemlist;

	prev = vmemlist;			/* Walk along free list	*/
	next = vmemlist->mnext;
	while ((next != NULL) && (next < block)) {
		prev = next;
		next = next->mnext;
	}

	if (prev == vmemlist) {		/* Compute top of previous block*/
		top = (uint32) NULL;
	} else {
		top = (uint32) prev + prev->mlength;
	}

	/* Ensure new block does not overlap previous or next blocks	*/

	if (((prev != vmemlist) && (uint32) block < top)
	    || ((next != NULL)	&& (uint32) block+nbytes>(uint32)next)) {
		restore(mask);
		debug("vfreeme: err, prev = 0x%08x, block = 0x%08x, top = 0x%08x\n", prev, block, top);
		return SYSERR;
	}

	vmemlist->mlength += nbytes;

	/* Either coalesce with previous block or add to free list */

	if (top == (uint32) block) { 	/* Coalesce with previous block	*/
		prev->mlength += nbytes;
		block = prev;
	} else {			/* Link into list as new node	*/
		block->mnext = next;
		block->mlength = nbytes;
		prev->mnext = block;
	}

	/* Coalesce with next block if adjacent */

	if (((uint32) block + block->mlength) == (uint32) next) {
		block->mlength += next->mlength;
		block->mnext = next->mnext;
	}
	restore(mask);

	return OK;
}
