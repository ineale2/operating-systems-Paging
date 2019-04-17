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
#ifndef EMPTY
#define EMPTY 	-1
#endif

/* Global clock algorithm status */
#define UM00		0
#define UM10		1
#define	UM11		2
#define SKIP_FRAME 	3

/* Inverted page table data structure */

typedef struct {
	uint32  nextFrame;		// Used when frame status is PAGE
	uint32  prevFrame;		// Used when frame status is PAGE
	int16	status;
	pid32 	pid;
	int32  vpn;
	uint32 	refCount; 		// When frame status is PTAB, this is the number of pages that are in memory
} frame_t;

frame_t ipt[NFRAMES];

/* Sempahore (used as mutex) for GCA */
sid32 gca_sem;

//Head and tail of FIFO queue for frame replacement
uint32 flistHead;
uint32 flistTail;

/* Frame allocation/freeing functions */
char* getNewFrame(uint32 type, pid32 pid, int32 vpn);
void  freeFrame(uint32 fr);
void  freeFrameFIFO(uint32 fr);
void  freeFrameGCA(uint32 fr);
void  freeProcFrames(pid32 pid);
char* getNewFrameFIFO(uint32 type, pid32 pid, int32 vpn);
char* getNewFrameGCA(uint32 type, pid32 pid, int32 vpn);

/* Initialization functions */
void init_frame(uint32 fr, uint32 type, pid32 pid, int32 vpn);
void init_frame_FIFO(uint32 fr);
void init_frame_GCA(uint32 fr);
void init_ipt(void);

/* Frame replacement functions */
void   evictFrame(uint32 fr);
uint32 pickFrame(void);
uint32 pickFrameGCA(void); 
uint32 pickFrameFIFO(void); 


/* Helper functions */
char*  frameNum2ptr(uint32 frameNum);
uint32 faddr2frameNum(char* faddr);
void   incRefCount(pt_t* pt);
void   decRefCount(pt_t* pt, pd_t* pd, uint32 pdi);
void   clearFrame(uint32 fr);

/* Global clock algorithm helper functions */
uint32 getAndSetUM(uint32);

/* Debug functions */
void printFrameQueue(void);
#endif // __IPT_H_
