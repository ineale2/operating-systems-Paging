Hello and welcome to my lab3 submission! The following is an (almost)  list of files
that I have modified, with comments where I feel they may be helpful in grading.

Files:
paging.c and paging.h
-This file contains the page fault handler and the bulk of the code to allocate and initialize pages
	 and page tables.

ipt.c and ipt.h (inverted page table)
-This file contains functions to allocate frames, pick a frame for eviction (under both GCA and FIFO), and
	some functions for frame cleanup.

pg.S
-This file contains all the assembly code that I have written for this lab. The page fault dispatcher is here.
	Note that these functions are defined externally in paging.h.

vgetmem.c and vfreemem.c
-These files are the virtual heap memory manager. I have chosen to store metadata in the virtual heap.

write_bs.c and read_bs.c
-These files were modified to retry a failed read/write up to 10 times before panicing. 

bs.c and bs.h
-These files manage the mappings from pages to backing store id's and offsets. Note that the backing store 
	IDs that a process holds are stored in the process table (proctab). Functions to allocate and deallocate
	backing stores are also contained in here, which are called at process creation and exit.

resched.c
vcreate.c
create.c
kill.c
initialize.c
process.h

