#include <xinu.h>


//TODO: When a process is deleted, return pages that it owns. 
//TODO: Remove vaddr2paddr calls

// Page fault handler. Called by pf_dispatcher (declared in pg.S)
void pf_handler(void){ //Interrupts are disabled by pf_dispatcher
	//global variable pfErrCode should be set by pf_dispatcher
	
	debug("In pf_handler\n");
	debug("Error Code: %d\n", pfErrCode);	
	debug("CR2: %d\n", readCR2());
	panic("Page fault\n");

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

