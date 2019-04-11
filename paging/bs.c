#include <xinu.h>
local int32 min(int32 a, int32 b);

status get_bs_info(pid32 pid, char* vaddr, bsd_t* bsd, uint32* offset){
	struct procent* prptr;
	uint32 vpn_offset;
	uint32 bs_offset;
	uint32 i;
	uint32 vpn;

	prptr = &proctab[pid];
	vpn = vaddr2vpn(vaddr);
	if(vpn < FRAME0){
		kprintf("get_bs_info: vpn = %d, vaddr = 0x%x, pid = %d\n", vpn, vaddr, pid);
		panic("get_bs_info failed\n");
	}
	vpn_offset = vpn - VPN0;
	
	i = vpn_offset/MAX_PAGES_PER_BS; 
	bs_offset = vpn_offset%MAX_PAGES_PER_BS;
	debug("get_bs_info: vaddr = 0x%08x, vpn = %d, pid = %d, vpn_offset = %d, bs_offset = %d, i = %d\n", vaddr, vpn, pid, vpn_offset, bs_offset, i);
	/* Write to pass-by-reference arguments */
	*bsd = prptr->pbsd[i];
	*offset = bs_offset;
	return OK;
}


status freeProcBS(struct procent* prptr){
	if(prptr->vh == NO_VHEAP){
		return OK;
	}
	int32 remain = (int32)prptr->hsize;
	int i = 0;
	bsd_t bsd;
	while(remain > 0){
		bsd = prptr->pbsd[i];
		deallocate_bs(bsd);
		free_bs_count++;
		remain -= MAX_PAGES_PER_BS;
		i++;
	}
	
	return OK;

}

status 	bs_init(struct procent* prptr, uint32 hsize){
	int i;
	for(i = 0; i < MAX_BS_ENTRIES; i++){
		prptr->pbsd[i] = (bsd_t)0;
	} 

	/* Allocate backing stores until all vheap pages are mapped to a backing store */
	int32 remain = (int32)hsize;
	int32 req;
	i = 0;
	while(remain > 0){
		req = min(remain, MAX_PAGES_PER_BS);
		prptr->pbsd[i] = allocate_bs(req);	
		free_bs_count--;
		remain -= MAX_PAGES_PER_BS;
		i++;
	}
	return OK;
}

local	int32 min(int32 a, int32 b){
	if(a < b) 
		return a;
	else
		return b;
}
