#include <xinu.h>

//TODO: When killed, remove frames from fifo queue
//TODO: Think of other scenarios when frame has to change

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
			init_frame(fr, type, vpn);
			debug("getNewFrame: fr = %d\n", fr);
			return frameNum2ptr(fr);
		}
		else{
			nextframe++;
		}
	}

	/* No frames are free */
	//TODO: Error checking for network failures
	// Select a frame to evict...
	fr = pickFrame(); 

	// Evict the frame...
	st = evictFrame(fr);
	if(st == SYSERR){
		kprintf("that sucks\n");
	}
	// Initialize the new frame
	init_frame(fr, type, pid, vpn);
	
	// Return a pointer to the new frame
	return frameNum2ptr(fr);
}

//This function looks up a backingstore number and offset for a given pid and vaddr. The return values are PBR
void get_bs_info(pid32 pid, char* vaddr, bsd_t* bsd, uint32* offset){

}  

//Clears previous information about the frame
void init_frame(uint32 fr, type, pid, vpn){
	ipt[fr].status 		= type;
	ipt[fr].pid			= pid;
	ipt[fr].vpn			= vpn;
	ipt[fr].refCount 	= 0;
}

void init_ipt(void){
	int i;
	for(i = 0; i < NFRAMES; i++){
		ipt[i].status 	= NOT_USED;
		ipt[i].pid 	  	= NO_PROCESS;
		ipt[i].vpn 	  	= NO_VPN;
		ipt[i].refCount = 0;
	}
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

uint32 pickFrameFIFO(void){

	return -1;
}

uint32 pickFrameGCA(void){ 

	return -1;
}

uint32 pickFrame(void){
	uint32 fr;
	if(currPolicy == FIFO){
		fr = pickFrameFIFO(void);
	}
	else{
		fr = pickFrameGCA(void);
	}
	return fr;
}
