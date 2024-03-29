	.text
	.globl  pf_dispatcher			/* Page fault			*/
	.globl  enablePaging 			/* Enables paging 		*/
	.globl  loadPD					/* One argument (pd addr) and puts into CR3 reg */
	.globl  readCR2					/* Returns cr2 register */
	.globl	readCR3
	.globl	readCR0
	.globl	invalPage
	.globl	writeCR2

pf_dispatcher:
	popl pfErrCode
	pushfl
	pushal	
	call pf_handler
	popal
	popfl
	iret

invalPage:
	movl 4(%esp), %eax
	invlpg (%eax)
	ret 

enablePaging:
	movl %cr0, %eax
	orl	 $0x80000001, %eax /* Set PE and PG bits to enable paging and enter protected mode */
	movl %eax, %cr0
	ret
	
loadPD:
	movl 4(%esp), %eax 
	movl %eax, %cr3
	ret

readCR0:
	movl %cr0, %eax
	ret
 
readCR2:
	movl %cr2, %eax
	ret

readCR3:
	movl %cr3, %eax
	ret

	.data
	.globl  pfErrCode
	pfErrCode:
		.long 0
