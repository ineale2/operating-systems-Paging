/* read_bs.c - read_bs */

#include <xinu.h>

/*----------------------------------------------------------------------------
 *  read_bs  -  This copies the pagenum'th page from the backing store
 *  referenced by ID store to memory pointed by dst. It returns OK on success,
 *  SYSERR otherwise. The first page of a backing store is page zero.
 *----------------------------------------------------------------------------
 */
local void dumpRDBLK(int i, uint32* p);
syscall read_bs (char *dst, bsd_t bs_id, uint32 page)
{
	int rd_blk = 0;
	char buf[RD_BLKSIZ] = {0};
	int i= 0;

	if(PAGE_SERVER_STATUS == PAGE_SERVER_INACTIVE){
		kprintf("Page server is not active\r\n");
		return SYSERR;
	}

	if (bs_id > MAX_ID || bs_id < MIN_ID) {
		kprintf("read_bs failed for bs_id %d and page number %d\r\n",
						bs_id,
						page);
		return SYSERR;
	}

	wait(bs_sem);

	if (bstab[bs_id].isopen == FALSE
			|| bstab[bs_id].npages <= page){
		kprintf("read_bs failed for bs_id %d and page number %d\r\n",
						bs_id,
						page);
		signal(bs_sem);
		return SYSERR;
	}
	signal(bs_sem);
	/*
	 * The first page for a backing store is page 0
	 * FIXME : Check id read on RDISK takes blocks from 0 ...
	 */
	rd_blk = (bs_id * RD_PAGES_PER_BS + page)*8;
	kprintf("\n================= READ BS ================\n");
	kprintf("read_bs : from rd_blk = %u, into addr 0x%08x, bsd = %d, page = %u \n", rd_blk, dst, bs_id, page);

	for(i=0; i< 8; i++){
		memset(buf, NULLCH, RD_BLKSIZ);
		kprintf("read_bs iteration [%d]\r\n", i);
		while(read(WRDISK, buf, rd_blk+i) == SYSERR){
			//panic("Could not read from backing store \r\n");
			kprintf("retying read_bs...\n");
		}
		//else{
			dumpRDBLK(i, (uint32*)(buf));
			memcpy((char *)(dst+i*RD_BLKSIZ), (char *)buf, RD_BLKSIZ);
	//	}
	}

	kprintf("\n=============== END READ BS ==============\n");

	return OK;
}

void dumpRDBLK(int i, uint32* p){
	kprintf("\n======= DUMPING BLK %d ========\n", i);
	frame_dump_flag = 1;
	kprintf("\n");
	uint32* end =(uint32*) ((char*)p + RD_BLKSIZ);
	int count = 0;
	while(p < end){
		kprintf("buf[%d]:0x%x\n", count, *p);
		p = p + 1;	
		count++;
	}
	kprintf("========= END DUMP BLK %d ======\n\n", i);
}
