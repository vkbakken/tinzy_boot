#include <stdint.h>
#include <stdbool.h>

#include "nrf.h"

#include "config.h"
#include "terminal.h"


void terminal_init(void)
{
    uint16_t countdown;

    TINZYBOOT_UART->PSEL.RTS = 0xFFFFffff;
    TINZYBOOT_UART->PSEL.CTS = 0xFFFFffff;
    TINZYBOOT_UART->PSEL.RXD = 0xFFFFffff;
    TINZYBOOT_UART->PSEL.TXD = TINZYBOOT_UART_TX_PIN_bp;

    TINZYBOOT_UART->EVENTS_RXSTARTED = 0;
    TINZYBOOT_UART->EVENTS_TXSTARTED = 0;
    TINZYBOOT_UART->EVENTS_TXSTOPPED = 0;
    TINZYBOOT_UART->TASKS_STOPRX = 1;

    TINZYBOOT_UART->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud115200;
    TINZYBOOT_UART->ENABLE = 8;
    TINZYBOOT_UART->EVENTS_TXSTARTED = 0;
    TINZYBOOT_UART->TASKS_STARTTX = 1;
    
    countdown = TINZYBOOT_MAX_POLL_ITER;
    while (TINZYBOOT_UART->EVENTS_TXSTARTED == 0) { if (!countdown--) break; }
    TINZYBOOT_UART->EVENTS_TXSTARTED = 0;
}


void terminal_deinit(void)
{
    uint16_t countdown;

    TINZYBOOT_UART->EVENTS_TXSTOPPED = 0;
    TINZYBOOT_UART->TASKS_STOPTX = 1;

    countdown = TINZYBOOT_MAX_POLL_ITER;
    while (TINZYBOOT_UART->EVENTS_TXSTOPPED == 0) { if (!countdown--) break; }

    TINZYBOOT_UART->ENABLE = 0;

    TINZYBOOT_UART->PSEL.TXD = 0xFFFFffff;
}


void terminal_send(const char *data, uint16_t len)
{
    TINZYBOOT_UART->EVENTS_ENDTX = 0;
    TINZYBOOT_UART->TXD.PTR = (uint32_t)data;
    TINZYBOOT_UART->TXD.MAXCNT = len;
    TINZYBOOT_UART->TASKS_STARTTX = 1;
    while (TINZYBOOT_UART->EVENTS_ENDTX != 0x1) { ; }
}
