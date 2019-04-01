#include <xinu.h>


//TODO: When a process is deleted, return pages that it owns. 

// Page fault handler. Called by pf_dispatcher (declared in pg.S)
void pf_handler(void){ //Interrupts are disabled by pf_dispatcher
	//global variable pfErrCode should be set by pf_dispatcher
	
	debug("In pf_handler\n");
	debug("Error Code: %d\n", pfErrCode);	
	debug("CR2: %d\n", readCR2());
	panic("Page fault\n");

}

void init_pd(pid32 pid){

	pd_t* pd = (pd_t*)getNewFrame(PDIR, pid, NO_VPN);
	char* addr;
	debug("init_pd: pid = %d\ninit_pd: pd start %x\n", pid, (void*)pd);
	int  j;
	pd_t* pd_ptr;
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
	debug("init_pd: pd end  %x\n", (void*)&pd[PAGEDIRSIZE]);

	// Initialize global page tables (pages 0 to 4095)
	for( j = 0; j < NUM_GLOBAL_PDE; j++){
		pd_ptr = &pd[j];
		pd_ptr->pd_pres 	= 1;
		pd_ptr->pd_write	= 1;
		//Allocate some space for a pt
		addr = getNewFrame(PTAB, pid, NO_VPN);
		set_PDE_addr(pd_ptr, addr); 
		debug("pd[j].pd_base = 0x%x\n", pd[j].pd_base);
		setup_id_paging((pt_t*)addr, (char*)(j << 22));
			
	}

	// Initialize device page tables (starting at 0x9000000, 1024 pages)
	pd_ptr = &pd[DEV_MEM_PD_INDEX];
	pd_ptr->pd_pres  = 1;
	pd_ptr->pd_write = 1;
	addr = getNewFrame(PTAB, pid, NO_VPN);
	set_PDE_addr(pd_ptr, addr); 
	setup_id_paging((pt_t*)addr, (char*)DEV_MEM_START);
	

}

void set_PTE_addr(pt_t* pt, char* addr){
	pt->pt_base = (uint32)addr >> 12;
}

void set_PDE_addr(pd_t* pd, char* addr){
	//This function deals with the complexity of the bit field storing the base address
	pd->pd_base = (uint32)addr >> 12;
}

void setup_id_paging(pt_t* pt, char* firstFrame){
	int i;
	//Only look at base address... get rid of lower bits. 
	char* frameAddr = (char*)((uint32) firstFrame & 0xfffff000);
	for(i = 0; i< PAGETABSIZE; i++){
		pt[i].pt_pres 	= 1;
		pt[i].pt_write 	= 1;
		set_PTE_addr(&pt[i], frameAddr);
		
		//Go to next frame
		//debug("idpg: pte = %d frameAddr = 0x%x, pt[i].pt_base = 0x%x\n", i, frameAddr, pt[i].pt_base);
		frameAddr += NBPG;
	}

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
