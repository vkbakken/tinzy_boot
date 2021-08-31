#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED


#define	APP_ENTRY	                    0x40000
#define FLASH_SIZE                      0x100000
#define FLASH_REGION_SIZE               (32 * 1024)
#define SRAM_SIZE          	            (256 * 1024)
#define SRAM_REGION_SIZE   	            (8 * 1024)

#define TINZYBOOT_MAX_POLL_ITER         (10)

#define TINZYBOOT_UART_TX_PIN_bp        (11)
#define TINZYBOOT_UART_TX_PIN_bm        (1 << TINZYBOOT_UART_TX_PIN_bp)
#define TINZYBOOT_UART                  NRF_UARTE0_S
#endif /*CONFIG_H_INCLUDED*/
