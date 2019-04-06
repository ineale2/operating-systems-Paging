#ifndef __IPT_H_
#define __IPT_H_

/* PID options */
#define GLOBAL 		-1
#define NO_PROCESS  -2

/* VPN options */
#define NO_VPN   -1

/* Status options */
#define NOT_USED   	0
#define PDIR		1
#define PTAB		2
#define PAGE		3

/* Frame FIFO Queue */
#define EMPTY 		-1

/* Inverted page table data structure */

typedef struct {
	uint32  nextFrame;		// Used when frame status is PAGE
	uint32  prevFrame;		// Used when frame status is PAGE
	int16	status;
	pid32 	pid;
	uint32  vpn;
	uint32 	refCount; 		// When frame status is PTAB, this is the number of pages that are in memory
} frame_t;

frame_t ipt[NFRAMES];

//Head and tail of FIFO queue for frame replacement
uint32 flistHead = EMPTY;
uint32 flistTail = EMPTY;

/* Frame allocation/freeing functions */
char* getNewFrame(uint32 type, pid32 pid, uint32 vpn);
void  freeFrame(uint32 fr);
void  freeFrameFIFO(uint32 fr);
void  freeFrameGCA(uint32 fr);
void freeProcFrames(pid32 pid);

/* Initialization functions */
void init_frame(uint32 fr, uint32 type, pid32 pid, uint32 vpn);
void init_frame_FIFO(uint32 fr, uint32 type, pid32 pid, uint32 vpn);
void init_frame_GCA(uint32 fr, uint32 type, pid32 pid, uint32 vpn);
void init_ipt(void);

/* Frame replacement functions */
void   evictFrame(uint32 fr, pid32 pid);
uint32 pickFrame(void);
uint32 pickFrameGCA(void); 
uint32 pickFrameFIFO(void); 

/* Helper functions */
status get_bs_info(pid32 pid, char* vaddr, bsd_t* bsd, uint32* offset);
char*  frameNum2ptr(uint32 frameNum);
void   incRefCount(pt_t* pt);
void   decRefCount(pt_t* pt);
void   clearFrame(uint32 fr);
#endif // __IPT_H_
