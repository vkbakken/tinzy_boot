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


static void errata_apply(void)
{
    /* Set all ARM SAU regions to NonSecure if TrustZone extensions are enabled.
     * Nordic SPU should handle Secure Attribution tasks */
    #if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
        SAU->CTRL |= (1 << SAU_CTRL_ALLNS_Pos);
    #endif
        
    /* Workaround for Errata 6 "POWER: SLEEPENTER and SLEEPEXIT events asserted after pin reset" found at the Errata document
     for your device located at https://infocenter.nordicsemi.com/index.jsp  */
    if (nrf91_errata_6()){
        NRF_POWER_S->EVENTS_SLEEPENTER = (POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_NotGenerated << POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_Pos);
        NRF_POWER_S->EVENTS_SLEEPEXIT = (POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_NotGenerated << POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_Pos);
    }

        /* Workaround for Errata 14 "REGULATORS: LDO mode at startup" found at the Errata document
            for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf91_errata_14()){
            *((volatile uint32_t *)0x50004A38) = 0x01ul;
            NRF_REGULATORS_S->DCDCEN = REGULATORS_DCDCEN_DCDCEN_Enabled << REGULATORS_DCDCEN_DCDCEN_Pos;
        }

        /* Workaround for Errata 15 "REGULATORS: LDO mode at startup" found at the Errata document
            for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf91_errata_15()){
            NRF_REGULATORS_S->DCDCEN = REGULATORS_DCDCEN_DCDCEN_Enabled << REGULATORS_DCDCEN_DCDCEN_Pos;
        }

    /* Workaround for Errata 20 "RAM content cannot be trusted upon waking up from System ON Idle or System OFF mode" found at the Errata document
    for your device located at https://infocenter.nordicsemi.com/index.jsp  */
    if (nrf91_errata_20()){
        *((volatile uint32_t *)0x5003AEE4) = 0xE;
    }

        /* Workaround for Errata 31 "XOSC32k Startup Failure" found at the Errata document
            for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf91_errata_31()){
            *((volatile uint32_t *)0x5000470Cul) = 0x0;
            *((volatile uint32_t *)0x50004710ul) = 0x1;
        }

        /* Trimming of the device. Copy all the trimming values from FICR into the target addresses. Trim
         until one ADDR is not initialized. */
        uint32_t index = 0;
        for (index = 0; index < 256ul && NRF_FICR_S->TRIMCNF[index].ADDR != 0xFFFFFFFFul; index++){
          #if defined ( __ICCARM__ )
              #pragma diag_suppress=Pa082
          #endif
          *(volatile uint32_t *)NRF_FICR_S->TRIMCNF[index].ADDR = NRF_FICR_S->TRIMCNF[index].DATA;
          #if defined ( __ICCARM__ )
              #pragma diag_default=Pa082
          #endif
        }

        /* Set UICR->HFXOSRC and UICR->HFXOCNT to working defaults if UICR was erased */
        if (uicr_HFXOSRC_erased() || uicr_HFXOCNT_erased()) {
          /* Wait for pending NVMC operations to finish */
          while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);

          /* Enable write mode in NVMC */
          NRF_NVMC_S->CONFIG = NVMC_CONFIG_WEN_Wen;
          while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);

          if (uicr_HFXOSRC_erased()){
            /* Write default value to UICR->HFXOSRC */
            uicr_erased_value = NRF_UICR_S->HFXOSRC;
            uicr_new_value = (uicr_erased_value & ~UICR_HFXOSRC_HFXOSRC_Msk) | UICR_HFXOSRC_HFXOSRC_TCXO;
            NRF_UICR_S->HFXOSRC = uicr_new_value;
            while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);
          }

          if (uicr_HFXOCNT_erased()){
            /* Write default value to UICR->HFXOCNT */
            uicr_erased_value = NRF_UICR_S->HFXOCNT;
            uicr_new_value = (uicr_erased_value & ~UICR_HFXOCNT_HFXOCNT_Msk) | 0x20;
            NRF_UICR_S->HFXOCNT = uicr_new_value;
            while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);
          }

          /* Enable read mode in NVMC */
          NRF_NVMC_S->CONFIG = NVMC_CONFIG_WEN_Ren;
          while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);

          /* Reset to apply clock select update */
          NVIC_SystemReset();
        }

       

        /* Allow Non-Secure code to run FPU instructions. 
         * If only the secure code should control FPU power state these registers should be configured accordingly in the secure application code. */
        SCB->NSACR |= (3UL << 10);
    
    /* Enable the FPU if the compiler used floating point unit instructions. __FPU_USED is a MACRO defined by the
    * compiler. Since the FPU consumes energy, remember to disable FPU use in the compiler if floating point unit
    * operations are not used in your code. */
    #if (__FPU_USED == 1)
      SCB->CPACR |= (3UL << 20) | (3UL << 22);
      __DSB();
      __ISB();
    #endif
}


int main(void)
{
    /*Do errata*/
    errata_apply();

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
