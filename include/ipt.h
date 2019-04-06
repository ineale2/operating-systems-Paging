#ifndef __IPT_H_
#define __IPT_H_

/* PID options */
#define GLOBAL -1

/* VPN options */
#define NO_VPN   -1

/* Status options */
#define NOT_USED   	0
#define PDIR		1
#define PTAB		2
#define PAGE		3

/* Inverted page table data structure */

typedef struct {
	uint32  nextFrameFIFO;
	int16	status;
	pid32 	pid;
	uint32  vpn;
	uint32 	refCount;
} frame_t;

frame_t ipt[NFRAMES];

char* getNewFrame(uint32 type, pid32 pid, uint32 vpn);

char* frameNum2ptr(uint32 frameNum);

void freeFrameNum(uint32 frnum);
void freeFrameAddr(char* fraddr);
void get_bs_info(pid32 pid, char* vaddr, bsd_t* bsd, uint32* offset);

uint32 pickFrame(void);
void init_frame(uint32 fr, type, pid, vpn);
status evictFrame(uint32);

uint32 pickFrameGCA(void); 
uint32 pickFrameFIFO(void); 

//Head of FIFO queue for frame replacement
uint32 fList;
#endif // __IPT_H_
