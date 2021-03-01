#include <stdint.h>
#include <stdbool.h>

#include "nrf.h"


#define	APP_ENTRY	        0x40000
#define FLASH_SIZE          0x100000
#define FLASH_REGION_SIZE   (32 * 1024)
#define SRAM_SIZE          	(256 * 1024)
#define SRAM_REGION_SIZE   	(8 * 1024)


extern unsigned int __etext;
extern unsigned int __data_start__;
extern unsigned int __bss_start__;
extern unsigned int __bss_end__;
extern unsigned int __stack;


void jump_ns(uint32_t addr);
void _start(void) __attribute__ ((section (".start")));


unsigned int __boot_exceptions[] __attribute__ ((section (".vec_tbl"), used)) = {
	[0]=	(unsigned int)&__stack,
    [1]=	(unsigned int)_start,
//    [2]=	(unsigned int)nmi_handler,
//    [3]=	(unsigned int)hard_fault_handler,
//    [4]=	(unsigned int)mem_fault_handler,
//    [5]=	(unsigned int)bus_fault_handler,
//    [6]=	(unsigned int)usage_fault_handler,
//    [7]=	0,
//    [8]=	0,
//	[9]=	0,
//    [10]=	0,
//    [11]=	(unsigned int)svc_call_handler,
//    [12]=	(unsigned int)debug_handler,
//    [13]=	0,
//    [14]=	(unsigned int)pend_sv_handler,
//    [15]=	(unsigned int)systick_handler
};


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


int main(void)
{
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


    /*Jump to non secure context*/
    //jump_ns(0x40000);

	while (1) {
		;
	}
}


void _start(void)
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
	__asm volatile( "cpsid i" ::: "memory" );
	while (1) { ; }
}
