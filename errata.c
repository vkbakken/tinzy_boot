#include <stdint.h>
#include <stdbool.h>

#include "nrf.h"


static void priv_apply(void)
{
#if 0
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

#endif       

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


void errata_apply(void)
{
    priv_apply();
}