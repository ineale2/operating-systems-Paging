#include <xinu.h>

//TODO: Remove all panic() statements that are not necessary
//TODO: Retry networking instructions

void freeFrame(uint32 fr){
	// Mark the frame as free
	if(currpolicy == FIFO && ipt[fr].status == PAGE){
		//Remove from the queue
		freeFrameFIFO(fr);
	}
	else if(currpolicy == GCA && ipt[fr].status == PAGE){
		freeFrameGCA(fr);
	}
	clearFrame(fr);
	return;
}

void freeFrameFIFO(uint32 fr){
	uint32 prev, next;
	prev = ipt[fr].prevFrame;
	next = ipt[fr].nextFrame;
	// Remove the frame from the queue
	if(fr == flistHead){
		ipt[next].prevFrame = EMPTY;
		flistHead = next;
	}
	else if(fr == flistTail){
		ipt[prev].nextFrame = EMPTY;
		flistTail = prev;
	}
	else{
		ipt[prev].nextFrame = next;
		ipt[next].prevFrame = prev;
	}
}

void freeProcFrames(pid32 pid){
	/* If the process is not using virtual memory, then it has not frames to free */
	if(proctab[pid].vh == NO_VHEAP){
		return;
	}
	int i;
	// Free all of frames belonging to the process pid
	for(i = 0; i< NFRAMES; i++){
		if(ipt[i].pid == pid){
			freeFrame(i);
		}
	}
}

void freeFrameGCA(uint32 fr){
	return;
}

char* getNewFrame(uint32 type, pid32 pid, uint32 vpn){
	uint32 i;
	static uint32 nextframe = 0;
	uint32 fr;

	/* Check all frame slots */
	for(i = 0; i < NFRAMES; i++){
		nextframe %= NFRAMES;
		if(ipt[nextframe].status == NOT_USED){
			// Found a free frame. Return a pointer to it. 
			fr = nextframe++;
			// Mark the frame as used
			init_frame(fr, type, pid, vpn);
			debug("getNewFrame: no eviction, free frame fr = %d\n", fr);
			return frameNum2ptr(fr);
		}
		else{
			nextframe++;
		}
	}
	// No frames are free, select a frame to evict
	fr = pickFrame(); 

	// Evict the frame
	evictFrame(fr);

	// Initialize the new frame
	init_frame(fr, type, pid, vpn);
	
	// Return a pointer to the new frame
	return frameNum2ptr(fr);
}

//Clears previous information about the frame
void init_frame(uint32 fr, uint32 type, pid32 pid, uint32 vpn){
	ipt[fr].status 		= type;
	ipt[fr].pid			= pid;
	ipt[fr].vpn			= vpn;
	ipt[fr].refCount 	= 0;
	ipt[fr].nextFrame   = EMPTY;
	ipt[fr].prevFrame	= EMPTY;

	/* If the frame is used for a page, then add the frame number to the queue */
	if(type == PAGE && currpolicy == FIFO){
		init_frame_FIFO(fr);
	}
	else if(type == PAGE &&  currpolicy == GCA){
		init_frame_GCA(fr);
	}
}

void init_frame_FIFO(uint32 fr){
	if(flistHead == EMPTY && flistTail == EMPTY){
		flistHead = fr;
		flistTail = fr;
	}
	else{
		// Enqueue at head
		ipt[fr].nextFrame = flistHead;
		ipt[fr].prevFrame = EMPTY;
		ipt[flistHead].prevFrame = fr;
		flistHead = fr;
	}
}

void init_frame_GCA(uint32 fr){
	return;
}



void evictFrame(uint32 fr){
	uint32 	vp;			/* Virtual page number of pg to be replaced */	
	pd_t*  	pd;			/* Page directory of current process 		*/
	pt_t*	pt;			/* Page table containing the replaced pg	*/
	char*	a;			/* First virtual address on page vp 		*/
	uint32  pdi;		/* Page directory index for the page table  */
	uint32  pti;		/* Page table index for page to be replaced */
	bsd_t 	bsd;		/* Backing store descriptor 			    */
	uint32  offset;		/* Offset into backing store 			    */
	status  s;			/* Status used for error checking 		    */
	char*   faddr;		/* Frame address 							*/
	pid32 	pid;		/* PID of process whose frame is evicted 	*/

	vp  = ipt[fr].vpn;
	pid = ipt[fr].pid;
	pd  = proctab[pid].pd;

	// Get address from vp
	a = (char*)(vp*NBPG);
	pdi = vaddr2pdi(a);
	pti = vaddr2pti(a);
	pt  = pdi2pt(pd, pdi);

	// Mark the page as not present
	pt[pti].pt_pres = 0;
	debug("Evicting frame fr = %d, which belongs to process %d and stores vpn = %d\n", fr, pid, vp);

	// If this page belongs to the current process, then it is in the TLB and needs to be invalidated
	if(currpid == pid){
		invalPage(a);
	}

	// Decrement reference count
	decRefCount(pt, pd, pdi);

	// Write page out to disk if necessary
	if(pt[pti].pt_dirty == 1){
		debug("evictFrame: writing dirty page\n");
		faddr = frameNum2ptr(fr);	
		debug("bs_info for WRITE\n");
		s = get_bs_info(pid, a, &bsd, &offset);
		if(s == SYSERR){
			kprintf("Dirty page not found in backing store, killing process %d\n", currpid);
			kill(currpid);
		}
		// Store changes in backing store
		s = write_bs(faddr, bsd, offset);
		if( s == SYSERR){
			panic("write_bs failed\n");
		}
	}
	hook_pswap_out(pid, vp, fr);
}

void decRefCount(pt_t* pt, pd_t* pd, uint32 pdi){
	uint32 fr = faddr2frameNum( (char*)pt );
	ipt[fr].refCount--;
	/* If the reference count for this page table reaches zero, none of its pages are in memory 	*/
	/* In this case, the page table itself should be removed from memory and PDIR set accordingly 	*/
	if(ipt[fr].refCount <= 0){
		debug("decRefCount: deleting page table PDI = %d from pd at 0x%08x, pt in fr = %d\n", pdi, pd, fr);
		kprintf("Page table deleted\n");
		freeFrame(fr);
		pd[pdi].pd_pres = 0;
		hook_ptable_delete(fr);
	}

}

void  incRefCount(pt_t* pt){
	uint32 fr = ((char*)(pt)-(char*)METADATA_START)/NBPG;
	debug("incRefCount: pt = 0x%x fr = %d\n",pt,fr);
	ipt[fr].refCount++;
}


uint32 pickFrameFIFO(void){
	uint32 fr;
	/* Dequeue from tail */
	if(flistTail == flistHead){
		//One element in queue
		fr = flistTail;
		flistTail = EMPTY;
		flistHead = EMPTY;
	}
	else{
		//More than one element in queue
		fr = flistTail;
		flistTail = ipt[fr].prevFrame;
		ipt[flistTail].nextFrame = EMPTY;
	}
	debug("pickFrameFIFO: picked fr = %d\n", fr);
	if(fr == EMPTY) {panic("empty frame returned!!\n");}
	return fr;
}

void init_ipt(void){
	int i;
	for(i = 0; i < NFRAMES; i++){
		clearFrame(i);
	}
}

void clearFrame(uint32 fr){
	ipt[fr].status 		= NOT_USED;
	ipt[fr].pid			= NO_PROCESS;
	ipt[fr].vpn			= NO_VPN;
	ipt[fr].nextFrame 	= EMPTY;
	ipt[fr].prevFrame 	= EMPTY;
	ipt[fr].refCount	= 0;
}


char* frameNum2ptr(uint32 frameNum){
	if(frameNum >= NFRAMES || frameNum < 0)
		kprintf("Bad frame num: %d\n", frameNum);
	char* temp = (char*)METADATA_START + frameNum*NBPG;
	debug("frameNum2ptr: fr %d = 0x%x\n", frameNum, (void*)temp);
	return temp;
}

uint32 faddr2frameNum(char* faddr){
	return (faddr - (char*)METADATA_START)/NBPG;
}

uint32 pickFrameGCA(void){ 

	return -1;
}

uint32 pickFrame(void){
	uint32 fr;
	if(currpolicy == FIFO){
		fr = pickFrameFIFO();
	}
	else{
		fr = pickFrameGCA();
	}
	return fr;
}
