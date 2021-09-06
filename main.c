#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#include "nrf.h"

#include "config.h"
#include "terminal.h"


extern unsigned int __etext;
extern unsigned int __data_start__;
extern unsigned int __bss_start__;
extern unsigned int __bss_end__;
extern unsigned int __stack;


extern void jump_ns(uint32_t addr);
extern int _snprintf(char * s, size_t n, const char *format, ...);
static void __start(void);
static void default_handler(void);


unsigned int __boot_exceptions[] __attribute__ ((section (".vec_tbl"), used)) = {
	[0]=	(unsigned int)&__stack, //Stack top
    [1]=	(unsigned int)__start, //Reset handler
    [2]=	(unsigned int)default_handler, //nmi
    [3]=	(unsigned int)default_handler, //hard fault
    [4]=	(unsigned int)default_handler, //mem fault
    [5]=	(unsigned int)default_handler, //bus fault
    [6]=	(unsigned int)default_handler, //usage_fault
};


static char terminal_out[128];


static void default_handler(void)
{
    while (1) { ; }
}


static void config_peripheral(uint8_t id, bool dma_present)
{
	NVIC_DisableIRQ(id);

	//if (usel_or_split(id)) {
		NRF_SPU_S->PERIPHID[id].PERM = SPU_PERIPHID_PERM_PRESENT_Msk | ~SPU_PERIPHID_PERM_SECATTR_Msk |\
						(dma_present ? SPU_PERIPHID_PERM_DMASEC_Msk : 0) |\
						SPU_PERIPHID_PERM_LOCK_Msk;
	//}

	/* Even for non-present peripherals we force IRQs to be routed
	 * to Non-Secure state.
	 */
	//irq_target_state_set(id, 0);
}


static void board_init(void)
{
    NRF_P0_S->DIRSET    = TINZYBOOT_UART_TX_PIN_bm;
}


int main(void)
{
    int length;


    /*Do errata*/
    errata_apply();

    board_init();

    terminal_init();

    length = _snprintf(terminal_out, sizeof(terminal_out), "TINZYBOOT STARTED\n");
    terminal_send(terminal_out, length);

    /*Configure flash*/
    for (int region = 0; region < (FLASH_SIZE / FLASH_REGION_SIZE); region++) {
        if (region < 2) {
            NRF_SPU_S->FLASHREGION[region].PERM = SPU_FLASHREGION_PERM_READ_Msk | SPU_FLASHREGION_PERM_WRITE_Msk\
												| SPU_FLASHREGION_PERM_EXECUTE_Msk | SPU_FLASHREGION_PERM_LOCK_Msk | SPU_FLASHREGION_PERM_SECATTR_Msk;
        } else {
            NRF_SPU_S->FLASHREGION[region].PERM = SPU_FLASHREGION_PERM_READ_Msk | SPU_FLASHREGION_PERM_WRITE_Msk\
												 | SPU_FLASHREGION_PERM_EXECUTE_Msk | SPU_FLASHREGION_PERM_LOCK_Msk | ~SPU_FLASHREGION_PERM_SECATTR_Msk;
        }
	}

    /*Configure SRAM*/
    for (int region = 0; region < (FLASH_SIZE / FLASH_REGION_SIZE); region++) {
		if (region < 2) {
            NRF_SPU_S->RAMREGION[region].PERM = SPU_RAMREGION_PERM_READ_Msk | SPU_RAMREGION_PERM_WRITE_Msk\
												| SPU_RAMREGION_PERM_EXECUTE_Msk | SPU_RAMREGION_PERM_LOCK_Msk | SPU_RAMREGION_PERM_SECATTR_Msk;
        } else {
            NRF_SPU_S->RAMREGION[region].PERM = SPU_RAMREGION_PERM_READ_Msk | SPU_RAMREGION_PERM_WRITE_Msk\
												 | SPU_RAMREGION_PERM_EXECUTE_Msk | SPU_RAMREGION_PERM_LOCK_Msk | ~SPU_RAMREGION_PERM_SECATTR_Msk;
        }
	}

    /*Configure peripherals*/
    

    /*Prepare tz stuff*/


    /*Clean up current context*/


    /*Jump to next image*/
    //jump_ns(0x40000);
}


static void __start(void)
{
	/*At this point it is assumed that the CPU has been reset, either from power-on
	 * or in some other way. Interrupts and CPU in virgin state assumed.
	 */

	/*Copy initialized data*/
	unsigned int *src = &__etext;
	unsigned int *dst = &__data_start__;
	unsigned int *end = &__bss_start__;

	while (dst != end) {
		*(dst++) = *(src++);
	}

	/*Clear BSS*/
	dst = &__bss_start__;
	end = &__bss_end__;
	while (dst != end) {
		*(dst++) = 0;
	}

	/*Jump to main*/
	main();

	/*If main was ever to return we stop here.*/
	asm volatile( "cpsid i" ::: "memory" );
	while (1) {
        asm volatile("dsb\n\t");
        asm volatile("wfi\n\t");
     }
}
