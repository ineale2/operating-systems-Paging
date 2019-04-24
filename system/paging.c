#include <xinu.h>

//TODO: For kill, need to write all frames to disk

// Page fault handler. Called by pf_dispatcher (declared in pg.S)
void pf_handler(void){ //Interrupts are disabled by pf_dispatcher
	uint32 	pti, pdi;		// Indexes into PT and PD
	char* 	a;				// Faulted virtual address
	uint32  vpn;			// Virtual page number of faulted address
	pd_t* 	pd;				// Page directory of current process
	pt_t* 	pt;				// Page table for faulted address
	bsd_t 	bsd;			// Backing store descriptor for not-present page
	uint32 	offset;			// Backing store offset for not-present page
	syscall e;				// For error checking
	char*	faddr;			// Physical address of new frame
	uint32  fr;				// Frame number used for replacement
	intmask mask;			// Stored interrupt mask

	mask = disable();
	// Increment page fault counter for instrumentation
	pfc++;
	a = (char*)readCR2();
	wait(pf_sem);

	//Get the faulted address
	pd = proctab[currpid].pd;

	//If the address is invalid, kill the process
	if(isInvalidAddr(a, currpid)){
		kprintf("Address 0x%x invalid for pid = %d\n", a, currpid);
		restore(mask);
		signal(pf_sem);
		kill(currpid);
		return; //Will never execute
	}

	//Convert to pti and pdi
	pti = vaddr2pti(a);
	pdi = vaddr2pdi(a);
	vpn = vaddr2vpn(a);
	pt  = pdi2pt(pd, pdi);

	// Need to handle two cases:
	// (1) Page table is not present and needs to be allocated
	// (2) Virtual page is not in RAM and needs to be brought in

	// Case 1: Check if page table is not present
	if(pd[pdi].pd_pres == 0){
		//Allocate a new page table at set the page directory entry
		pt = newPageTable(currpid);

		//Point the page directory to the new page table and mark it as present
		set_PDE_addr(&pd[pdi], (char*)pt); 
		pd[pdi].pd_pres = 1;
	}
	// Now, actions to handle case1 and case2 are identical
	// Obtain a free frame
	faddr = getNewFrame(PAGE, currpid, vpn);		
	fr = faddr2frameNum(faddr);

	// Using the backing store map, find the store s and page offset o which correspond to pti
	get_bs_info(currpid, a, &bsd, &offset);

	// Copy the page in the backing store to the new frame
	e = read_bs(faddr, bsd, offset);
	if(e == SYSERR){
		panic("read_bs failed\n");
	}
	// Increment reference count of the frame that holds pt
	incRefCount(pt);

	// Update the page table entry, point it to the frame and set bits	
	pt[pti].pt_pres = 1;
	set_PTE_addr(&pt[pti], faddr);

	hook_pfault(currpid, a, vpn, fr + FRAME0); 

	// Release lock
	signal(pf_sem);
	restore(mask);
}

void init_gpt(){
	// Initialize global page tables (pages 0 to 4095)
	int j;
	for( j = 0; j < NUM_GLOBAL_PT; j++){
		gpt[j] = getNewFrame(PTAB, GLOBAL, NO_VPN);
		//Allocate some space for a pt
		setup_id_paging((pt_t*)gpt[j], (char*)(j << 22));
		hook_ptable_create(faddr2frameNum(gpt[j]) + FRAME0);
	}
	// Initialize device page tables (starting at 0x9000000, 1024 pages)
	dpt = getNewFrame(PTAB, GLOBAL, NO_VPN);
	setup_id_paging((pt_t*)dpt, (char*)DEV_MEM_START);
}

void setup_id_paging(pt_t* pt, char* firstFrame){
	int i;
	//Only look at base address... get rid of lower bits. 
	char* frameAddr = (char*)((uint32) firstFrame & 0xfffff000);
	for(i = 0; i< PAGETABSIZE; i++){
		pt[i].pt_pres 	= 1;
		pt[i].pt_write 	= 1;
		pt[i].pt_user	= 0;
		pt[i].pt_pwt 	= 0;
		pt[i].pt_pcd 	= 0;
		pt[i].pt_acc 	= 0;
		pt[i].pt_dirty  = 0;
		pt[i].pt_mbz 	= 0;
		pt[i].pt_global = 0;
		pt[i].pt_avail 	= 0;
	
		set_PTE_addr(&pt[i], frameAddr);

		//Go to next frame
		frameAddr += NBPG;
	}

}

pt_t* newPageTable(pid32 pid){
	char* faddr = getNewFrame(PTAB, pid, NO_VPN);
	if(faddr == (char*)SYSERR){
		return (pt_t*)SYSERR;
	}
	pt_t* pt = (pt_t*)faddr;
	int i;
	for( i = 0; i< PAGETABSIZE; i++){
		pt[i].pt_pres 		= 0;
		pt[i].pt_write 		= 1;
		pt[i].pt_user		= 0;
		pt[i].pt_pwt 		= 0;
		pt[i].pt_pcd 		= 0;
		pt[i].pt_acc 		= 0;
		pt[i].pt_dirty  	= 0;
		pt[i].pt_mbz 		= 0;
		pt[i].pt_global		= 0;
		pt[i].pt_avail		= 0;
	}
	hook_ptable_create(faddr2frameNum(faddr) + FRAME0);
	return pt;
}

status init_pd(pid32 pid){
	wait(pf_sem);
	pd_t* pd = (pd_t*)getNewFrame(PDIR, pid, NO_VPN);
	signal(pf_sem);
	if(pd == (pd_t*)SYSERR){
		return SYSERR;
	}
	pd_t* pd_ptr;
	int  j;
	// Put location of page directory into proctab
	proctab[pid].pd = pd;

	//Initialize all entries in the page directory
	for( j = 0; j< PAGEDIRSIZE; j++){
		pd_ptr = &pd[j];
		pd_ptr->pd_pres 	= 0;
		pd_ptr->pd_write	= 1;
		pd_ptr->pd_user	    = 0;
		pd_ptr->pd_pwt	    = 0;
		pd_ptr->pd_pcd  	= 0;
		pd_ptr->pd_acc   	= 0;
		pd_ptr->pd_mbz   	= 0;
		pd_ptr->pd_fmb   	= 0;
		pd_ptr->pd_global	= 0;
		pd_ptr->pd_avail	= 0;
		pd_ptr->pd_base		= 0;
	}
	//Set global page tables
	for( j = 0; j < NUM_GLOBAL_PT; j++){
		set_PDE_addr(&pd[j], gpt[j]);
		pd[j].pd_write 		= 1;
		pd[j].pd_pres 		= 1;
		pd[j].pd_global 	= 1;
	}
	//Set device page table
	set_PDE_addr(&pd[DEV_MEM_PD_INDEX], dpt);
	pd[DEV_MEM_PD_INDEX].pd_write 		= 1;
	pd[DEV_MEM_PD_INDEX].pd_pres 		= 1;
	pd[DEV_MEM_PD_INDEX].pd_global 		= 1;

	return OK;

}

void walkPDIR(void){
	char* vaddr = 0x00000000;
	debug("walking pdir of pid = %d\n", currpid);
	uint32 counter = 0;
	
	while(vaddr < (char*)0x00FFFFFF){
		vaddr2paddr(vaddr, vaddr);
		vaddr = vaddr + 1;
		counter++;
		if(counter%1000000 == 0){
			debug("0x%x\n", vaddr);
		}
	}
	vaddr = (char*)0x90000000;
	while(vaddr < (char*)0x903FFFFF){
		vaddr2paddr(vaddr, vaddr);
		vaddr = vaddr + 1;
		counter++;
		if(counter%1000000 == 0){
			debug("0x%x\n", vaddr);
		}
	}


}

char* vaddr2paddr(char* vaddr, char* expected){
	char* paddr;
	pd_t* pd = proctab[currpid].pd;
	pt_t* pt;
	uint32 pdi, pti;
	uint32 offset;
	
	offset = vaddr2offset(vaddr); 
	pdi = vaddr2pdi(vaddr);
	pti = vaddr2pti(vaddr);
	pt  = (pt_t*)(pd[pdi].pd_base << 12); 
	paddr = (char*)(pt[pti].pt_base << 12);
	paddr =(char*)( (uint32)paddr | offset);
	
	if(expected != paddr){
		debug("========= ERROR PADDR != EXPECTED  ===\n");
		kprintf("vaddr: 0x%x paddr: 0x%x, expected 0x%x\n", vaddr, paddr, expected);
		debug("pdi: %d, pti: %d, offset: %d\n", pdi, pti, offset);
		debug("pt = 0x%x, pte = 0x%x\n", pt, (char*)(pt[pti].pt_base << 12));
		debug("vaddr:\n");
		dump32((long)vaddr);
		debug("pde:\n");
		printPDE(pd, pdi);
		debug("pte:\n");
		printPTE(pt, pti);
		//printPT(pt);
		dump32((long)pt[pti].pt_base << 12);
		debug("========== END VADDR2PADDR =======\n\n\n");
	}
	return paddr;

}

void set_PTE_addr(pt_t* pt, char* addr){
	pt->pt_base = (uint32)addr >> 12;
//	debug("setPTEaddr: addr = 0x%x, pt_base = 0x%x\n", addr, pt->pt_base);
}

void set_PDE_addr(pd_t* pd, char* addr){
	//This function deals with the complexity of the bit field storing the base address
	pd->pd_base = (uint32)addr >> 12;
}

void dump32(unsigned long n) {
  int i;

  for(i = 31; i>=0; i--) {
    kprintf("%02d ",i);
  }

  kprintf("\n");

  for(i=31;i>=0;i--)
    kprintf("%d  ", (n&(1<<i)) >> i);

  kprintf("\n");
}

uint32 vaddr2offset(char* vaddr){
	return ((uint32)vaddr & 0x00000FFF);
}

uint32 vaddr2pdi(char* vaddr){
	return ((uint32)vaddr & 0xFFC00000) >> 22;
}

uint32 vaddr2pti(char* vaddr){
	return ((uint32)vaddr & 0x003FF000) >> 12;
}
pt_t* pdi2pt(pd_t* pd, uint32 pdi){
	return (pt_t*)(pd[pdi].pd_base << 12);
}

uint32 vaddr2vpn(char* vaddr){
	return ((uint32)vaddr) >> 12;
}
uint32 pde2pdi(pd_t* pd){
	return (pd->pd_base & 0xFFC00) >> 10;
}

uint32 pte2pti(pt_t* pt){
	return (pt->pt_base & 0x003FF);
}


void printPD(pd_t* pd){

	int i;
	debug("printPD: START\n");
	for(i = 0; i< PAGEDIRSIZE; i++){
		printPDE(pd, i);
	}
	debug("printPD: END\n");

}

void printPT(pt_t* pt){
	int i;
	debug("printPT: START, pt address = 0x%08x, in frame fr = %d\n", pt, faddr2frameNum( (char*)pt ));
	for(i = 0; i< PAGETABSIZE; i++){
		printPTE(pt, i);
	}
	debug("printPT: END\n");
}

void dumpmem(void){
	kprintf("dumping mem\n");
	uint32* p = (uint32*)(0x00400000);
	while(p < (uint32*)0x00406000){
		kprintf("0x%x:0x%x\n", p, *p);
		p = p + 1;	
	}
	kprintf("dump finished\n");
}
void printPDE(pd_t* pd, uint32 pdi){
	pd = pd + pdi;
	kprintf("PDE: 0x%08x, base = 0x%08x, P=%d W=%d U=%d PWT=%d PCD=%d ACC=%d MBZ=%d FMB=%d G=%d AVL=%d\n",
		*((uint32*)(pd)), pd->pd_base << 12, pd->pd_pres, pd->pd_write, pd->pd_user, pd->pd_pwt, pd->pd_pcd,
		pd->pd_acc, pd->pd_mbz, pd->pd_fmb, pd->pd_global, pd->pd_avail);
}

void printPTE(pt_t* pt, uint32 pti){
	pt = pt + pti;
	kprintf("PTE: 0x%08x, base = 0x%08x, P=%d W=%d U=%d PWT=%d PCD=%d ACC=%d D=%d MBZ=%d G=%d AVL=%d\n",
		*((uint32*)(pt)), pt->pt_base << 12, pt->pt_pres, pt->pt_write, pt->pt_user, pt->pt_pwt, pt->pt_pcd,
		pt->pt_acc, pt->pt_dirty, pt->pt_mbz, pt->pt_global, pt->pt_avail);
}

void dumpframe(uint32 fr){
	kprintf("\n================== DUMPING FRAME %d ==================\n", fr);
	kprintf("\n");
	uint32* p = (uint32*)frameNum2ptr(fr);;
	uint32* end =(uint32*) ((char*)p + NBPG);
	while(p < end){
		kprintf("0x%x:0x%x\n", p, *p);
		p = p + 1;	
	}
	kprintf("=================== END DUMP FRAME %d ================\n\n", fr);
}

int isInvalidAddr(char* a, pid32 pid){
	struct procent* prptr = &proctab[pid];
	char* vheapEnd = (char*)VHEAP_START + NBPG*prptr->hsize;
	return a >= vheapEnd;
}

void printErrCode(uint32 e){
	debug("errCode = %d", e);
	if(e & 0x00000001) 
		debug(", page level protection violation");
	else
		debug(", page not present");

	if(e & 0x00000002)
		debug(", write err");
	else
		debug(", read err");

	if(e & 0x00000003)
		debug(", supervisor mode\n");
	else
		debug(", user mode\n");
}
