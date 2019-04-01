#include <xinu.h>


void freeFrameNum(uint32 frnum){
	return;
}


void freeFrameAddr(char* fraddr){
	return;
}

char* getNewFrame(uint32 type, pid32 pid, uint32 vpn){
	uint32 i;
	static uint32 nextframe = 0;
	uint32 fr;
	status st;

	/* Check all frame slots */
	for(i = 0; i < NFRAMES; i++){
		nextframe %= NFRAMES;
		if(ipt[nextframe].status == NOT_USED){
			// Found a free frame. Return a pointer to it. 
			fr = nextframe++;
			// Mark the frame as used
			ipt[fr].status 	= type;
			ipt[fr].pid		= pid;
			ipt[fr].vpn 	= vpn;
			debug("getNewFrame: fr = %d\n", fr);
			return frameNum2ptr(fr);
		}
		else{
			nextframe++;
		}
	}

	/* No frames are free */
	panic("No free frames\n");
	//TODO: Error checking for network failures
	// Select a frame to evict...
	fr = pickFrame(); 

	// Evict the frame...
	st = evictFrame(fr);
	if(st == SYSERR)
		kprintf("that sucks\n");

	// Return a pointer to the new frame
	return frameNum2ptr(fr);
}

char* frameNum2ptr(uint32 frameNum){
	if(frameNum >= NFRAMES || frameNum < 0)
		kprintf("Bad frame num: %d\n", frameNum);
	char* temp = (char*)0x00400000 + frameNum*NBPG;
	debug("fr %d = 0x%x\n", frameNum, (void*)temp);
	return temp;
}

status evictFrame(uint32 fr){
	/* Check for dirty bit. Need to write frame back */
	return 0;
}

uint32 pickFrame(void){
	return 0;
}
