#include <xinu.h>
#define numRegions 4
void writemem(uint32* p, uint32 numPages, uint32 count);
void readmem(uint32* p, uint32 numPages, uint32 count);
extern uint32 get_test_value2(uint32 *addr);
uint32 get_test_value3(uint32 *addr, uint32 seed);
void proc(uint32 inc, uint32 numPages);
void manyMem(uint32 numPtrs, uint32 size);
void stopper2(uint32 numPages);
void stopper(uint32 numPages);
void looper(uint32, uint32, sid32);
void test1(void);
void test2(void);
void test3(void);
void test4(void);
void test5(void);
void test6(void);
void test7(void);
//TODO: Use memory, free memory, use memory again (make sure state is consisient after)
//TODO: Also in this test, verify number of page faults is what is expected

extern void page_policy_test(void);

process	main(void)
{
  srpolicy(GCA);
	kprintf("after sr policy\n");

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

  page_policy_test();

  	kprintf("TESTING START\n");
	test1();
	test2();
	test4();
	test5();
	//TODO: Figure out why test6 cannot pass if sleep(10) is in there. rdsbufalloc panics, but doesnt always show the message
	//test6();
	test7();

//	test3();
	kprintf("END OF ALL TESTS\n");
  return OK;
}

//Two processes 
void test1(void){
	kprintf("\n=================== TEST 1 ================\n");
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
	kprintf("\n=================== TEST 2 ================\n");
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
	kprintf("\n=================== TEST 3 ================\n");
	kprintf("Testing a large allocation\n");
	ASSERT(NFRAMES > 250 && NFRAMES < 400);
	uint32 numPages = 8*200; //max number of pages
	pid32 p1 = vcreate(proc, INITSTK, numPages, INITPRIO, "proc1", 2, 500, numPages);  	
  	resume(p1);
	sleep(45);
	kprintf("=============== END OF TEST 3 =============\n");
}

// Killed process does not mess up frame queue
void test4(void){
	kprintf("\n=================== TEST 4 ================\n");
	kprintf("Testing that killing a process leaves queue in functional state\n");
	ASSERT(NFRAMES < 30);
	uint32 numPages = 50; //max number of pages
	pid32 p1 = vcreate(stopper, INITSTK, numPages, INITPRIO, "proc1", 1, numPages);  	
  	resume(p1);
	pid32 p2 = vcreate(stopper, INITSTK, numPages, INITPRIO, "proc1", 1, numPages);  	
	resume(p2);
	sleep(5);
	kprintf("Both processes should have suspended\n");
	kprintf("Killing pid = %d\n", p1);
	kill(p1);
	kprintf("Resuming pid = %d\n", p2);
	resume(p2);
	sleep(5);
	kprintf("=============== END OF TEST 4 =============\n");
}

// After page table delete, process can find the right memory
void test5(void){
	kprintf("\n=================== TEST 5 ================\n");
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

void test6(void){
	kprintf("\n=================== TEST 6 ================\n");
	kprintf("Testing error calls and edge cases\n");
	ASSERT(NFRAMES < 30);
	int numPages = 1;
	pid32 pid[NFRAMES];
	//9th vcreate should error out	
	int i;
	for(i = 0; i < 8; i++){
		pid[i] = vcreate(stopper2, INITSTK/16, numPages, INITPRIO, "proc1", 1, numPages);  
		if(pid[i] == (pid32)SYSERR)
			kprintf("TEST FAIL, vcreate %d returned error\n", i);
	}
	kprintf("Err message should display...\n");
	if( SYSERR != vcreate(stopper2, INITSTK/16, numPages, INITPRIO, "proc1", 1, numPages)){
		kprintf("TEST FAIL, 9th vcreate succeeded\n");
	}
	for(i = 0; i < 8; i++){
		if(SYSERR == kill(pid[i]))
			kprintf("TEST FAIL, could not kill pid %d\n", pid[i]);
	}
	// Now run a normal process
	numPages = 350;
	kprintf("Creating normal process\n");
	pid32 p1 = vcreate(proc, INITSTK, numPages, INITPRIO, "proc1", 2, 512, numPages);  	
	resume(p1);
	sleep(10);

	kprintf("\nProcess %d should have finished by now\n", p1);
	// Now create so many processes that there are not enough frames for PDIR
	//kprintf("Now calling create()\n");
	for(i = 0; i < NFRAMES; i++){
		//kprintf("%d ", i);
		pid[i] = create(stopper2, INITSTK/16, INITPRIO, "proc1", 1, -1); 
		if(pid[i] == (pid32)SYSERR){
			kprintf("\nTEST PASS: No space for PDIR\n");
			break;
		}
	}
	int j;
	for(j = 0; j < i; j++){
		if(SYSERR == kill(pid[j])){
			kprintf("\nTEST FAIL, culd not kill pid %d\n", pid[j]);
		}
	}
	if(i == NFRAMES) kprintf("TEST FAIL: All create() calls suceeded\n");


	kprintf("=============== END OF TEST 6 =============\n");

}

void test7(void){
	kprintf("================== TEST 7 === =============\n");
	char* policy[5] = {"blank","blank", "blank", "FIFO", "GCA "}; 
	uint32 starttime = clktime;
	uint32 pfcstart = pfc;
	kprintf("START: POLICY = %s, NFRAMES = %d, Page Fault Count = %d, Backing Store Write Count = %d, Time = %d\n", policy[currpolicy], NFRAMES, pfc-pfcstart, 0, clktime-starttime);
	//Create many processes
	uint32 numPages = 32;
	uint32 numLoops = 1000;
	sid32 s = semcreate(0);
	if(s == SYSERR) kprintf("FAIL: semcreate returned syserr\n");
	pid32 p1 = vcreate(looper, INITSTK, numPages, INITPRIO, "proc1", 3, numPages, numLoops, s);  	
	resume(p1);

	if(SYSERR == wait(s))
		kprintf("FAIL: wait failed on sid %d\n", s);
	kprintf("END  : POLICY = %s, NFRAMES = %d, Page Fault Count = %d, Backing Store Write Count = %d, Time = %d\n", policy[currpolicy], NFRAMES, pfc-pfcstart, 0, clktime-starttime);
	kprintf("=============== END OF TEST 7 =============\n");
}

void looper(uint32 numPages, uint32 loops, sid32 s){
	int i;
	int req = numPages/numRegions;
	uint32* ptr[numRegions];
	for( i = 0; i < numRegions; i++){
		ptr[i] = (uint32*)vgetmem(req*NBPG);
		if(ptr[i] == (uint32*)SYSERR)
			panic("FAIL: looper vgetmem call failed\n");
	}
	//Alternate between reading and writing memory
	char flags[numRegions] = {1,1,1,1};
	kprintf("Looper start: ");
	for( i = 0; i < loops; i++){
		//kprintf("Loop %d\n", i);
		//Frequently used memory
		if(flags[0]) 
			writemem(ptr[1], req, i);
		else 
			readmem(ptr[1], req, i-1);
		flags[0] = !flags[0];

		//Less frequently used memory
		if(i%5 == 0){
			if(flags[1]) 
				writemem(ptr[2], req, i);
			else
				readmem(ptr[2], req, i-5);
			flags[1] = !flags[1];
		}
	
		// Rarely used memory
		if(i%10 == 0){
			if(flags[2])
				writemem(ptr[3], req, i);
			else
				readmem(ptr[3], req, i-10);
			flags[2] = !flags[2];
		}

		// Almost never used memory
		if(i%100 == 0){
			if(flags[3])
				writemem(ptr[4], req, i);
			else
				readmem(ptr[4], req, i-100);
			flags[3] = !flags[3];
			kprintf("%d ", i/100);
		}
		
		
	}

	for( i = 0; i< numRegions; i++){
		if(SYSERR == vfreemem((char*)ptr[i], req))
			panic("TEST FAIL: looper, vfreemem returned SYSERR\n");
	}
	kprintf("\nLooper finished\n");
	if(SYSERR == signal(s))
		kprintf("FAIL: signal on sid = %d failed\n", s);
} 

void readmem(uint32* p, uint32 numPages, uint32 count){
//	kprintf("readmem:  p = 0x%08x\n", p);
	int i;
	uint32 val;
	for(i = 0; i < numPages*NBPG; i+=4){
		val = get_test_value3(p, count);
		if(val != *p){
			kprintf("Addr: 0x%08x, data: 0x%08x, expected: 0x%08x\n", p, *p, val);
			panic("FAIL: Checking memory failed\n");
		}
		p++;
	}
}

void writemem(uint32* p, uint32 numPages, uint32 count){
//	kprintf("writemem: p = 0x%08x\n", p);
	int i;
	for(i = 0; i < numPages*NBPG; i+=4){
		*p= get_test_value3(p, count);
		p++;
	}
}

void stopper2(uint32 numPages){
	uint32* start = (uint32*)vgetmem(numPages*NBPG);
	uint32* p = start;
	uint32 i;
	uint32 val;
	kprintf("WRITE: stopper2, currpid = %d\n", currpid);
	for(i = 0; i < numPages*NBPG; i+=4){
		*p = get_test_value2(p);
		p++;
	}
	kprintf("SUSPEND: stopper2, currpid = %d\n", currpid);
	suspend(currpid);
	kprintf("RESUMED: stopper2, currpid = %d\n", currpid);
	kprintf("CHECKING: stopper2, currpid = %d\n", currpid);
	p = start;
	for(i = 0; i < numPages*NBPG; i+=4){
		val = get_test_value2(p);
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
		if(i%(10*NBPG) == 0) kprintf("%d ", count+=10);
	}
	kprintf("\nSUSPEND: pid = %d\n", currpid);
	suspend(currpid);
	kprintf("RESUMED: pid = %d\n", currpid);

	p = start;
	for(i = 0 ; i < numPages*NBPG ; i+=4*inc){
		*p = get_test_value2(p);	
		p+=inc;
		if(i%(10*NBPG) == 0) kprintf("%d ", count+=10);
	}
	kprintf("\nCHECKING: pid = %d\n", currpid);
	
	p = start;
	count = 0;
	for(i = 0 ; i < numPages*NBPG ; i+=4*inc){
		val = get_test_value2(p);
		if(*p != val){
			kprintf("FAIL: 0x%08x, data = 0x%08x, expected 0x%08x\n", p, *p, val);
			panic("stopper: Test fail\n");
		}
		p+=inc;
		if(i%(10*NBPG) == 0) kprintf("%d ", count+=10);
	}
	if(SYSERR == vfreemem((char*)start, numPages*NBPG)){
		kprintf("vfreemem failed\n");
		panic("test failed\n");
	}
	kprintf("\nstopper: pid %d exit\n", currpid);
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
			*ptr = get_test_value2(ptr);
		} 
	}

	kprintf("READ: manyMem, pid = %d\n", currpid);
	for(i = 0; i < numPtrs; i++){
		ptr = p[i];
		end = (uint32*)( (char*)ptr + size );
		for( ; ptr < end; ptr++){
			val = get_test_value2(ptr);
			if(*ptr != val){
				kprintf("FAIL: 0x%08x, data = 0x%08x, expected 0x%08x\n", ptr, *ptr, val);
				//Dump all memory in this frame
				char* faddr = vaddr2paddr((char*)ptr, (char*)0);	
				kprintf("Next 3 expected values: 0x%08x, 0x%08x, 0x%08x\n", get_test_value2(ptr + 1), get_test_value2(ptr + 2), get_test_value2(ptr + 3));
				dumpframe(faddr2frameNum(faddr));	
				panic("manyMem: Test fail\n");
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
		*p = get_test_value2(p);	
		p+=inc;
		if(i%(10*NBPG) == 0) kprintf("%d ", count+=10);
	}
	kprintf("\nCHECKING: pid = %d\n", currpid);
	
	p = start;
	count = 0;
	for(i = 0 ; i < numPages*NBPG ; i+=4*inc){
		val = get_test_value2(p);
		if(*p != val){
			kprintf("FAIL: 0x%08x, data = 0x%08x, expected 0x%08x\n", p, *p, val);
			//Dump all memory in this frame
			char* faddr = vaddr2paddr((char*)p, (char*)0);	
			kprintf("Next 3 expected values: 0x%08x, 0x%08x, 0x%08x\n", get_test_value2(p + 1), get_test_value2(p + 2), get_test_value2(p + 3));
			dumpframe(faddr2frameNum(faddr));	
			panic("proc: Test fail\n");
		}
		p+=inc;
		if(i%(10*NBPG) == 0) kprintf("%d ", count+=10);
	}
	if(SYSERR == vfreemem((char*)start, numPages*NBPG)){
		kprintf("vfreemem failed\n");
		panic("test failed\n");
	}
	kprintf("proc: pid %d exit\n", currpid);
}

uint32 get_test_value2(uint32 *addr) {
  static uint32 v1 = 0x12345678;
  static uint32 v2 = 0xdeadbeef;
  return (uint32)addr + v1*currpid + ((uint32)addr * v2);
}

uint32 get_test_value3(uint32 *addr, uint32 seed) {
  static uint32 v1 = 0x12345678;
  static uint32 v2 = 0xdeadbeef;
  return (uint32)addr*seed + v1*currpid + ((uint32)addr * v2);
}
