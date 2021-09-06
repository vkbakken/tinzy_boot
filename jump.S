.cpu cortex-m33
.syntax unified
.thumb

.globl jump_ns
.text
.thumb_func
.type jump_ns, function

jump_ns:
	lsrs	r0, r0, #1
	lsls	r0, r0, #1
	dsb		sy
	isb		sy
	bxns	r0


.globl jump_sec
.text
.thumb_func
.type jump_sec, function
jump_sec:
	lsrs	r0, r0, #1
	lsls	r0, r0, #1
	dsb	sy
	isb	sy
	bx	r0

