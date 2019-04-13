#include <xinu.h>
extern uint32 get_test_value(uint32 *addr);
void proc(uint32 inc, uint32 numPages);
void manyMem(uint32 numPtrs, uint32 size);
void stopper2(uint32 numPages);
void stopper(uint32 numPages);
void test1(void);
void test2(void);
void test3(void);
void test4(void);
void test5(void);

extern void page_policy_test(void);

process	main(void)
{
  srpolicy(FIFO);

  /* Start the network */
  /* DO NOT REMOVE OR COMMENT BELOW */
#ifndef QEMU
  netstart();
#endif

  /*  TODO. Please ensure that your paging is activated 
      by checking the values from the control register.
  */

  /* Initialize the page server */
  /* DO NOT REMOVE OR COMMENT THIS CALL */
  psinit();

  //page_policy_test();

	//test1();
	test2();
	//test3();
	//test4();
	//test5();
  return OK;
}

//Two processes 
void test1(void){
	kprintf("=================== TEST 1 ================\n");
	kprintf("Testing two processes\n");
	ASSERT(NFRAMES < 30);
	uint32 numPages = 50;
	pid32 p1 = vcreate(proc, INITSTK, numPages, INITPRIO, "proc1", 2, 32, numPages);  	
  	resume(p1);
	numPages = 10;
	pid32 p2 = vcreate(proc, INITSTK, numPages, INITPRIO, "proc2", 2, 16, numPages);  	
	resume(p2);
	sleep(5);
	kprintf("=============== END OF TEST 1 =============\n");
}

//Many vgetmem and vfreemem
void test2(void){
	kprintf("=================== TEST 2 ================\n");
	kprintf("Testing many vgetmem calls\n");
	ASSERT(NFRAMES < 30);
	uint32 numPages = 100;
	pid32 p1 = vcreate(manyMem, INITSTK, numPages, INITPRIO, "proc1", 2, 24, 2000);  	
  	resume(p1);
	pid32 p2 = vcreate(manyMem, INITSTK, numPages, INITPRIO, "proc2", 2, 1000, 50);  	
	resume(p2);
	sleep(5);
	kprintf("=============== END OF TEST 2 =============\n");

}


//Large allocation
void test3(void){
	kprintf("=================== TEST 3 ================\n");
	kprintf("Testing a large allocation\n");
	ASSERT(NFRAMES > 250 && NFRAMES < 400);
	uint32 numPages = 8*200; //max number of pages
	pid32 p1 = vcreate(proc, INITSTK, numPages, INITPRIO, "proc1", 2, 500, numPages);  	
  	resume(p1);
	sleep(20);
	kprintf("=============== END OF TEST 3 =============\n");
}

// Killed process does not mess up frame queue
void test4(void){
	kprintf("=================== TEST 4 ================\n");
	kprintf("Testing that killing a process leaves queue in functional state\n");
	ASSERT(NFRAMES < 30);
	uint32 numPages = 50; //max number of pages
	pid32 p1 = vcreate(stopper, INITSTK, numPages, INITPRIO, "proc1", 1, numPages);  	
  	resume(p1);
	pid32 p2 = vcreate(stopper, INITSTK, numPages, INITPRIO, "proc1", 1, numPages);  	
	resume(p2);
	sleep(10);
	kprintf("Both processes should have suspended\n");
	kprintf("Killing pid = %d\n", p1);
	kill(p1);
	kprintf("Resuming pid = %d\n", p2);
	resume(p2);
	sleep(10);
	kprintf("=============== END OF TEST 4 =============\n");
}

// After page table delete, process can find the right memory
void test5(void){
	kprintf("=================== TEST 5 ================\n");
	kprintf("Testing process can find memory after page table delete\n");
	ASSERT(NFRAMES < 30);
	uint32 numPages = 1;
	pid32 p1 = vcreate(stopper2, INITSTK, numPages, INITPRIO, "proc1", 1, numPages);  	
  	resume(p1);
	sleep(2);
	numPages = 40;
	pid32 p2 = vcreate(proc, INITSTK, numPages, INITPRIO, "proc1", 2, 512, numPages);  	
	resume(p2);
	sleep(5);
	
	kprintf("Other process should have completed...\n");
	kprintf("Resuming pid = %d\n", p1);
	resume(p1);
	sleep(5);
	kprintf("=============== END OF TEST 5 =============\n");

}

void stopper2(uint32 numPages){
	uint32* start = (uint32*)vgetmem(numPages*NBPG);
	uint32* p = start;
	uint32 i;
	uint32 val;
	kprintf("WRITE: stopper2, currpid = %d\n", currpid);
	for(i = 0; i < numPages*NBPG; i+=4){
		*p = get_test_value(p);
		p++;
	}
	kprintf("SUSPEND: stopper2, currpid = %d\n", currpid);
	suspend(currpid);
	kprintf("RESUMED: stopper2, currpid = %d\n", currpid);
	kprintf("CHECKING: stopper2, currpid = %d\n", currpid);
	p = start;
	for(i = 0; i < numPages*NBPG; i+=4){
		val = get_test_value(p);
		if(*p != val){
			kprintf("FAIL: 0x%08x, data = 0x%08x, expected 0x%08x\n", p, *p, val);
			panic("Test fail\n");
		}
		p++;
	}
	if(SYSERR == vfreemem((char*)start, numPages*NBPG)){
		kprintf("vfreemem failed\n");
		panic("test failed\n");
	}
	kprintf("DONE: stopper2, currpid = %d\n", currpid);
}

void stopper(uint32 numPages){
	uint32 inc = 4;
	uint32* start = (uint32*)vgetmem(numPages*NBPG);
	uint32* p = start;	
	uint32 val;
	uint32 i;
	uint32 count = 0;
	kprintf("WRITING: pid = %d\n", currpid);
	for(i = 0 ; i < numPages*NBPG ; i+=4*inc){
		*p = 0xDEADBEEF;	
		p+=inc;
		if(i%NBPG == 0) kprintf("%d ", count++);
	}
	kprintf("SUSPEND: pid = %d\n", currpid);
	suspend(currpid);
	kprintf("RESUMED: pid = %d\n", currpid);

	for(i = 0 ; i < numPages*NBPG ; i+=4*inc){
		*p = get_test_value(p);	
		p+=inc;
		if(i%NBPG == 0) kprintf("%d ", count++);
	}
	kprintf("\nCHECKING: pid = %d\n", currpid);
	
	p = start;
	count = 0;
	for(i = 0 ; i < numPages*NBPG ; i+=4*inc){
		val = get_test_value(p);
		if(*p != val){
			kprintf("FAIL: 0x%08x, data = 0x%08x, expected 0x%08x\n", p, *p, val);
			panic("Test fail\n");
		}
		p+=inc;
		if(i%NBPG == 0) kprintf("%d ", count++);
	}
	if(SYSERR == vfreemem((char*)start, numPages*NBPG)){
		kprintf("vfreemem failed\n");
		panic("test failed\n");
	}
	kprintf("proc: pid %d exit\n", currpid);
}


void manyMem(uint32 numPtrs, uint32 size){
	uint32* p[numPtrs];
	int i;
	uint32 val;
	for(i = 0; i < numPtrs; i++){
		p[i] = (uint32*)vgetmem(size);
		if(p[i] == (uint32*)SYSERR){
			panic("manyMem: vgetmem failed\n");
		} 
	}
	uint32* ptr;
	uint32* end;
	kprintf("WRITE: manyMem, pid = %d\n", currpid);
	for(i = 0; i < numPtrs; i++){
		ptr = p[i];
		end = (uint32*)( (char*)ptr + size );
		for( ; ptr < end; ptr++){
			*ptr = get_test_value(ptr);
		} 
	}

	kprintf("READ: manyMem, pid = %d\n", currpid);
	for(i = 0; i < numPtrs; i++){
		ptr = p[i];
		end = (uint32*)( (char*)ptr + size );
		for( ; ptr < end; ptr++){
			val = get_test_value(ptr);
			if(*ptr != val){
				kprintf("FAIL: 0x%08x, data = 0x%08x, expected 0x%08x\n", ptr, *ptr, val);
				panic("Test fail\n");
			}
		} 
	}
	
	int st;
	for(i = 0; i < numPtrs; i++){
		st = vfreemem((char*)p[i], size);
		if(st == SYSERR){
			kprintf("manyMem: vfreemem failed\n");
			panic("test failed\n");
		}
	}
	kprintf("manyMem: pid = %d done!\n", currpid);

}

void proc(uint32 inc, uint32 numPages){
	uint32* start = (uint32*)vgetmem(numPages*NBPG);
	uint32* p = start;	
	uint32 val;
	uint32 i;
	uint32 count = 0;
	kprintf("WRITING: pid = %d\n", currpid);
	for(i = 0 ; i < numPages*NBPG ; i+=4*inc){
		*p = get_test_value(p);	
		p+=inc;
		if(i%NBPG == 0) kprintf("%d ", count++);
	}
	kprintf("\nCHECKING: pid = %d\n", currpid);
	
	p = start;
	count = 0;
	for(i = 0 ; i < numPages*NBPG ; i+=4*inc){
		val = get_test_value(p);
		if(*p != val){
			kprintf("FAIL: 0x%08x, data = 0x%08x, expected 0x%08x\n", p, *p, val);
			panic("Test fail\n");
		}
		p+=inc;
		if(i%NBPG == 0) kprintf("%d ", count++);
	}
	if(SYSERR == vfreemem((char*)start, numPages*NBPG)){
		kprintf("vfreemem failed\n");
		panic("test failed\n");
	}
	kprintf("proc: pid %d exit\n", currpid);
}
/*
uint32 get_test_value(uint32 *addr) {
  static uint32 v1 = 0x12345678;
  static uint32 v2 = 0xdeadbeef;
  return (uint32)addr + v1 + ((uint32)addr * v2);
}
*/
