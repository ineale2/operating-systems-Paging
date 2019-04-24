/* write_bs.c - write_bs */

#include <xinu.h>

/*----------------------------------------------------------------------------
 *  write_bs  -  This copies a page pointed to by src to the pagenum'th page of
 *  the backing store referenced by store. It returns OK on success, SYSERR
 *  otherwise.
 *----------------------------------------------------------------------------
 */

local void  dumpRDBLK(int i, uint32* p);

syscall write_bs (char *src, bsd_t bs_id, uint32 page)
{
	uint32 rd_blk = 0;
	char buf[RD_BLKSIZ] = {0};
	int i=0;

	if(PAGE_SERVER_STATUS == PAGE_SERVER_INACTIVE){
		kprintf("Page server is not active\r\n");
		return SYSERR;
	}

	if (bs_id > MAX_ID || bs_id < MIN_ID) {
		kprintf("write_bs failed for bs_id %d and page number %d\r\n",
						bs_id,
						page);
		return SYSERR;
	}

	wait(bs_sem);

	if (bstab[bs_id].isopen == FALSE
			|| page >= bstab[bs_id].npages){
		kprintf("write_bs failed for bs_id %d and page number %d\r\n",
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
	kprintf("\n=============== WRITE  BS ================\n");
	kprintf("write_bs: into rd_blk = %u, from addr 0x%08x, bsd = %d, page = %u \n", rd_blk, src, bs_id, page);

	for(i=0; i< 8; i++){
		kprintf("write_bs iteration [%d]\r\n", i);
		memcpy((char *)buf, (char *)(src+i*RD_BLKSIZ),  RD_BLKSIZ);
		dumpRDBLK(i,(uint32*)buf); 
		while(write(WRDISK, buf, rd_blk+i) == SYSERR){
			kprintf("retying write_bs...\n");
			//panic("Could not write to backing store \r\n");
		}
	}
	kprintf("\n============= END WRITE BS ==============\n");

	return OK;
}

void dumpRDBLK(int i, uint32* p){
	kprintf("\n================== DUMPING BLK %d ==================\n", i);
	frame_dump_flag = 1;
	kprintf("\n");
	uint32* end =(uint32*) ((char*)p + RD_BLKSIZ);
	int count = 0;
	while(p < end){
		kprintf("buf[%d]:0x%x\n", count, *p);
		p = p + 1;	
		count++;
	}
	kprintf("=================== END DUMP BLK %d ================\n\n", i);
}
