#include <xinu.h>


//TODO: When a process is deleted, return pages that it owns. 
//TODO: Remove vaddr2paddr calls
//TODO: Think about interrupts disabling and enabling
//TODO: For pf_handler with new page table, do you also get frame for data?

// Page fault handler. Called by pf_dispatcher (declared in pg.S)
void pf_handler(void){ //Interrupts are disabled by pf_dispatcher
	uint32 	pti, pdi;		// Indexes into PT and PD
	char* 	a;				// Faulted virtual address
	pd_t* 	pd;				// Page directory of current process
	pt_t* 	pt;				// Page table for faulted address
	bsd_t 	bsd;			// Backing store descriptor for not-present page
	uint32 	offset;			// Backing store offset for not-present page
	syscall e;				// For error checking
	char*	faddr;			// Physical address of new frame

	debug("In pf_handler\n");
	debug("Error Code: %d\n", pfErrCode);	
	debug("CR2: 0x%x\n", readCR2());
	//Get the faulted address
	a = readCR2();
	pd = proctab[currpid].pd;

	//If the address is invalid, kill the process
	if(isInvalidAddr(a, currpid)){
		kprintf("Address 0x%x invalid for pid = %d\n", a, currpid);
		//TODO: Enable interrupts here?
		kill(currpid);
	}

	//Convert to pti and pdi
	pti = vaddr2pti(a);
	pdi = vaddr2pdi(a);
	pt  = pdi2pt(pd, pdi);

	// Need to handle two cases:
	// (1) Page table is not present and needs to be allocated
	// (2) Virtual page is not in RAM and needs to be brought in

	// Case 1: Check if page table is not present
	if(pd[pdi].pd_pres == 0){
		//Allocate a new page table at set the page directory entry
		set_PDE_addr(&pd[pdi], (char*)newPageTable(currpid)); 
		//TODO: Allocate another frame for data? 
		return;
	}
	else if(pd[pdi].pd_pres && !pt[pti].pt_pres){
		// Case 2: Virtual page is in the backing store
		// Using the backing store map, find the store s and page offset o which correspond to pti
		get_bs_info(currpid, a, &bsd, &offset);
		// Increment reference count of the frame that holds pt
		incRefCount(fr);
		// Obtain a free frame
		faddr = getNewFrame(PAGE, currpid, pti);		
		// Copy the page in the backing store to the new frame
		e = read_bs(faddr, bsd, offset);
		if(e == SYSERR){
			panic("read_bs failed\n");
		}
		// Update the page table entry, point it to the frame and set bits	
		pt[pti].pt_pres = 1;
		set_PTE_addr(&pt[pti], faddr);
	}
	else{
		panic("Bad news bears\n");
	} 
}

void init_gpt(){
	// Initialize global page tables (pages 0 to 4095)
	int j;
	for( j = 0; j < NUM_GLOBAL_PT; j++){
		gpt[j] = getNewFrame(PTAB, GLOBAL, NO_VPN);
		debug("gpt[%d] = 0x%x\n", j, gpt[j]);
		//Allocate some space for a pt
		setup_id_paging((pt_t*)gpt[j], (char*)(j << 22));
	}
	// Initialize device page tables (starting at 0x9000000, 1024 pages)
	dpt = getNewFrame(PTAB, GLOBAL, NO_VPN);
	debug("dpt = 0x%x\n", dpt);
	setup_id_paging((pt_t*)dpt, (char*)DEV_MEM_START);
}

void setup_id_paging(pt_t* pt, char* firstFrame){
	int i;
	//Only look at base address... get rid of lower bits. 
	char* frameAddr = (char*)((uint32) firstFrame & 0xfffff000);
	//uint32* temp;
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
		//kprintf("After  Addr: 0x%x :: Data: 0x%x\n", temp, *temp);
		//debug("idpg: pte = %d frameAddr = 0x%x, pt[i].pt_base = 0x%x\n", i, frameAddr, pt[i].pt_base);
		//debug("pt[%d].pt_base = %d\n", i, pt[i].pt_base);
		
		//kprintf("==============\n\n");
		//Go to next frame
		frameAddr += NBPG;
	}

}

pt_t* newPageTable(pid32 pid){
	pt_t* pt = (pt_t*)getNewFrame(PTAB, pid, NO_VPN);
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
	return pt;
}

void init_pd(pid32 pid){

	pd_t* pd = (pd_t*)getNewFrame(PDIR, pid, NO_VPN);
	pd_t* pd_ptr;
	debug("init_pd: pid = %d\ninit_pd: pd start %x\n", pid, (void*)pd);
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
	}
	//Set device page table
	set_PDE_addr(&pd[DEV_MEM_PD_INDEX], dpt);
	pd[DEV_MEM_PD_INDEX].pd_write 		= 1;
	pd[DEV_MEM_PD_INDEX].pd_pres 		= 1;
	debug("init_pd: pd end  %x\n", (void*)&pd[PAGEDIRSIZE]);


}

void walkPDIR(void){
	char* vaddr = 0x00000000;
	debug("walking pdir of pid = %d\n", currpid);
	uint32 counter = 0;
	
	while(vaddr < (char*)0x00FFFFFF){
		vaddr2paddr(vaddr);
		vaddr = vaddr + 1;
		counter++;
		if(counter%1000000 == 0){
			debug("0x%x\n", vaddr);
		}
	}
	vaddr = (char*)0x90000000;
	while(vaddr < (char*)0x903FFFFF){
		vaddr2paddr(vaddr);
		vaddr = vaddr + 1;
		counter++;
		if(counter%1000000 == 0){
			debug("0x%x\n", vaddr);
		}
	}


}

char* vaddr2paddr(char* vaddr){
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
	
	if(vaddr != paddr){
		debug("==================================\n");
		debug("vaddr: 0x%x paddr: 0x%x\n", vaddr, paddr);
		debug("pdi: %d, pti: %d, offset: %d\n", pdi, pti, offset);
		debug("pt = 0x%x, pte = 0x%x\n", pt, (char*)(pt[pti].pt_base << 12));
		debug("vaddr:\n");
		dump32((long)vaddr);
		debug("pte:\n");
		dump32((long)pt[pti].pt_base << 12);
		debug("==================================\n\n\n");
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
uint32 pde2pdi(pd_t* pd){
	return (pd->pd_base & 0xFFC00) >> 10;
}

uint32 pte2pti(pt_t* pt){
	return (pt->pt_base & 0x003FF);
}


void printPD(pd_t* pd_ptr){

	int i;
	pd_t* pd;
	debug("PDE NUM :  PT Addr  :P:W\n");
	debug("-------   ---------- - -\n"); 
	for(i = 0; i< PAGEDIRSIZE; i++){
		pd = &pd_ptr[i];
		debug("PDE %04d: 0x%x %d %d\n", i, pd->pd_base << 12, pd->pd_pres, pd->pd_write);
	}

}

void printPT(pt_t* pt_ptr){
	int i;
	pt_t* pt;
	debug("PTE NUM :  FR Addr  :P:W\n");
	debug("-------   ---------- - -\n"); 
	for(i = 0; i< PAGETABSIZE; i++){
		pt = &pt_ptr[i];
		debug("PTE %04d: 0x%x %d %d\n", i, pt->pt_base << 12, pt->pt_pres, pt->pt_write);
	}
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

