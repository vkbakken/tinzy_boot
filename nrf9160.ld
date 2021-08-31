MEMORY
{
	flash   (rx)  : ORIGIN = 0x00000000, LENGTH = 8k
	sram_sec   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}


SECTIONS
{
	. = 0x0;
	.vec_tbl . : {
		*vec_tbl.o(.text);
	} > flash
	
	.text : {
		*(.exception);
		*(.text*);
	} > flash

	.rodata : {
		*(.rodata*);
	} > flash

	. = .;
	. = ALIGN(4);
    __etext = .;
	.data : AT (__etext)
	{
		__data_start__ = .;
		*(.data*);
		. = ALIGN(4);
        /* All data end */
        __data_end__ = .;
	} > sram_sec


	.bss : {
		. = ALIGN(4);
        __bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
        __bss_end__ = .;
	} > sram_sec

    PROVIDE(__stack = ORIGIN(sram_sec) + LENGTH(sram_sec));
}
