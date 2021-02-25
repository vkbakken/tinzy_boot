#include <stdint.h>


#define	APP_ENTRY	0x40000


void jump_ns(uint32_t addr);



int main(void)
{
    /*Do A*/

    /* Do B*/

    /*Jump to non secure context*/
    jump_ns(0x1);
}
