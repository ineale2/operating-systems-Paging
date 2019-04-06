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

/* Inverted page table data structure */

typedef struct {
	int16	status;
	pid32 	pid;
	uint32  vpn;
} frame_t;

frame_t ipt[NFRAMES];

char* getNewFrame(uint32 type, pid32 pid, uint32 vpn);

char* frameNum2ptr(uint32 frameNum);

void freeFrameNum(uint32 frnum);
void freeFrameAddr(char* fraddr);

uint32 pickFrame(void);

status evictFrame(uint32);
#endif // __IPT_H_
