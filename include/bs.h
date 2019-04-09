#ifndef __BS_H_
#define __BS_H_

status get_bs_info(pid32 pid, char* vaddr, bsd_t* bsd, uint32* offset);

status 	bs_init(struct procent* prptr, uint32 hsize);

status freeProcBS(struct procent* prptr);


#endif //__BS_H_
