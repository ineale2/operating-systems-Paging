	.text
	.globl  pf_dispatcher			/* Page fault			*/
	.globl  enablePaging 			/* Enables paging 		*/
	.globl  loadPD					/* One argument (pd addr) and puts into CR3 reg */
	.globl  readCR2					/* Returns cr2 register */

pf_dispatcher:
	cli		
	pushl %eax
	movl 8(%esp), %eax
	movl %eax, pfErrCode 
	pushal	
	call pf_handler
	popal
	popl %eax
	addl $4, %esp	/* Need to remove error code from stack */
	sti
	iret

enablePaging:
	movl %cr0, %eax
	or	 $0x80000001, %eax /* Set PE and PG bits to enable paging and enter protected mode */
	movl %eax, %cr0
	ret
	
loadPD:
	movl 4(%esp), %eax 
	movl %eax, %cr3
	ret
 
readCR2:
	movl %cr2, %eax
	ret
	
	.data
	.globl  pfErrCode
	pfErrCode:
		.long 0
