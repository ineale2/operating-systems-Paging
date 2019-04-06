/* paging.h */

/* Helpful control macros for debugging and printing */
#define XDEBUG 		1

#if XDEBUG
#define debug(...) kprintf(__VA_ARGS__)
#else
#define debug(...) 
#endif

#ifndef __PAGING_H_
#define __PAGING_H_

/* Structure for a page directory entry */

typedef struct {
	unsigned int pd_pres	: 1;		/* page table present?		*/
	unsigned int pd_write : 1;		/* page is writable?		*/
	unsigned int pd_user	: 1;		/* is use level protection?	*/
	unsigned int pd_pwt	: 1;		/* write through cachine for pt? */
	unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
	unsigned int pd_acc	: 1;		/* page table was accessed?	*/
	unsigned int pd_mbz	: 1;		/* must be zero			*/
	unsigned int pd_fmb	: 1;		/* four MB pages?		*/
	unsigned int pd_global: 1;		/* global (ignored)		*/
	unsigned int pd_avail : 3;		/* for programmer's use		*/
	unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {
	unsigned int pt_pres	: 1;		/* page is present?		*/
	unsigned int pt_write : 1;		/* page is writable?		*/
	unsigned int pt_user	: 1;		/* is use level protection?	*/
	unsigned int pt_pwt	: 1;		/* write through for this page? */
	unsigned int pt_pcd	: 1;		/* cache disable for this page? */
	unsigned int pt_acc	: 1;		/* page was accessed?		*/
	unsigned int pt_dirty : 1;		/* page was written?		*/
	unsigned int pt_mbz	: 1;		/* must be zero			*/
	unsigned int pt_global: 1;		/* should be zero in 586	*/
	unsigned int pt_avail : 3;		/* for programmer's use		*/
	unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

/* Keeping track of physical frames */


#define PAGEDIRSIZE	1024
#define PAGETABSIZE	1024

#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame		*/

#ifndef NFRAMES
#define NFRAMES		3072	/* number of frames		*/
#endif

#define MAP_SHARED 1
#define MAP_PRIVATE 2

#define FIFO 3
#define GCA 4

#define MAX_ID		7		/* You get 8 mappings, 0 - 7 */
#define MIN_ID		0

#define DEV_MEM_PD_INDEX 		576
#define NUM_GLOBAL_PT 			4
#define METADATA_START 			0x00400000
#define PD_START 				METADATA_START
#define PT_START 				(METADATA_START + NPROC*PAGEDIRSIZE)
#define VIRTUAL_HEAP_START 		0x01000000
#define DEV_MEM_START 			0x90000000

#define PF_INTERRUPT_NUM		14
extern int32	currpolicy;
extern int32	pfErrCode;

/* 4 global page tables */
char* gpt[NUM_GLOBAL_PT];
/* Global device page table */
char* dpt; 

void init_pd(pid32 pid);
void init_gpt(void);

void pf_handler(void);
void set_PTE_addr(pt_t* pt, char* addr);
void set_PDE_addr(pd_t* pd, char* addr);
void setup_id_paging(pt_t* pt, char* firstFrame);
void dump32(unsigned long n);
void walkPDIR(void);
char* vaddr2paddr(char* vaddr);

/* The following functions are defined in system/pg.S */
extern void pf_dispatcher(void);
extern void enablePaging(void); 			/* Enables paging 		*/
extern void loadPD(pd_t*);				/* One argument (pd loc) and puts into CR3 reg */
extern uint32 readCR2(void);
extern uint32 readCR3(void);
extern uint32 readCR0(void);
extern uint32 enPg(void);

/* PDIR and PTAB conversion functions */
uint32 vaddr2pdi(char* vaddr);
uint32 vaddr2pti(char* vaddr);
uint32 vaddr2offset(char* vaddr);
uint32 pde2pdi(pd_t* pd);
uint32 pte2pti(pt_t* pt);

void dumpmem(void);

#endif // __PAGING_H_
