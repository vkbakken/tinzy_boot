#ifndef TERMINAL_H_INCLUDED
#define TERMINAL_H_INCLUDED


#include <stdint.h>
#include <stdbool.h>


void terminal_init(void);
void terminal_deinit(void);
void terminal_send(const char *data, uint16_t len);
#endif /*TERMINAL_H_INCLUDED*/
