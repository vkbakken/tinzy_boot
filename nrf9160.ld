MEMORY
{
	flash   (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00000
	sram0   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}


ENTRY(md_init)
SECTIONS
{
	. = 0x0;
	.start . : {
		*start.o(.text);
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
	} > sram0


	.bss : {
		. = ALIGN(4);
        __bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
        __bss_end__ = .;
	} > sram0
}
