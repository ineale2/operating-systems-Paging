#include <xinu.h>

void init_gpt(void){
	pd     = (uint32*)0x00400000;
	gpt[0] = (uint32)0x00401000;
	gpt[1] = (uint32)0x00402000;
	gpt[2] = (uint32)0x00403000;
	gpt[3] = (uint32)0x00404000;
	dpt    = (uint32)0x00405000;


	int i, j;
	uint32* curr_pt;
	for(i = 0; i<4; i++){
		pd[i] = (gpt[i] | 3);
		//kprintf("pd[%d] = 0x%x\n", i, pd[i]);
		curr_pt = (uint32*)gpt[i];
		for( j = 0; j < PAGETABSIZE; j++){
			curr_pt[j] = (i*PAGETABSIZE + j) << 12;
			curr_pt[j] |= 3;
			//kprintf("pt[%d] = 0x%x\n", j, curr_pt[j]);
		}
	}
	pd[576] = dpt | 3;
	//kprintf("pd[576] = 0x%x\n", pd[576]);
	i = 576;
	curr_pt = (uint32*)dpt;
	for( j = 0; j < PAGETABSIZE; j++){
		curr_pt[j] = (i*PAGETABSIZE + j) << 12;
		curr_pt[j] |= 3;
	}
	//kprintf("Walking PDIR\n");
	//walkPDIR();
	//kprintf("Done walking PDIR\n");
	dumpmem();
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

void walkPDIR(void){
	uint32* vaddr = (uint32*)0x00000000;
	uint32 count;
	while(vaddr < (uint32*) 0x00FFFFFF){
		vaddr2paddr(vaddr);
		vaddr = vaddr + 1;
		count++;
		if(count%1000000 == 0){
			kprintf("0x%x\n", vaddr);
		}
	}

	vaddr = (uint32*)0x90000000;
	while(vaddr < (uint32*) 0x903FFFFF){
		vaddr2paddr(vaddr);
		vaddr = vaddr + 1;
		count++;
		if(count%1000000 == 0){
			kprintf("0x%x\n", vaddr);
		}
	}
}

char* vaddr2paddr(uint32* vaddr){
	uint32 va = (uint32)(vaddr);
	uint32 offset = (va & 0x00000FFF);
	uint32 pti = (va & 0x003FF000) >> 12;
	uint32 pdi = (va & 0xFFC00000) >> 22;
	uint32 i   = (va & 0xFFC00000);
	uint32* pt = (uint32*)(pd[pdi] & 0xFFFFF000);
	uint32 temp1 = pt[pti] & 0xFFFFF000;
	uint32 temp2 = temp1 | offset;
	uint32* paddr =(uint32*)temp2;
	
	if(paddr != vaddr){
		kprintf("==========================\n");
		kprintf("vaddr: 0x%x paddr: 0x%x\n", vaddr, paddr);
		kprintf("pdi: %d pti: %d offset: %d\n", pdi, pti, offset);
		kprintf("pt: 0x%x pt[pti] = 0x%x temp1 = 0x%x temp2 = 0x%x\n", pt, pt[pti], temp1, temp2);
		kprintf("i : 0x%x va: 0x%x vaddr: 0x%x\n", i, va, vaddr);
		kprintf("==========================\n\n");

	}
	return (char*)0;
}


