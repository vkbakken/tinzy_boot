#TOOLCHAIN_LOC	:= /opt/gcc-arm-none-eabi/8-2018-q4-major/bin
TOOLCHAIN_LOC	:= /opt/gnuarmemb/bin
CC				:= $(TOOLCHAIN_LOC)/arm-none-eabi-gcc

PRJ				:= tinzy_boot

CFLAGS			:= -mthumb -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mlittle-endian -munaligned-access
CFLAGS			+= -g -nostdlib -nostdinc -fshort-enums -fno-builtin-printf -ffreestanding -flto
CFLAGS			+= 

CFILES			:= main.c
#CFILES			+= ./nrfx/mdk/system_nrf9160.c
CINC			:= -I ./nrfx/mdk/ -I ./CMSIS_5/CMSIS/Core/Include

CFLAGS			:= -Wall -Os -mcpu=cortex-m33 -mlittle-endian -mfloat-abi=soft -mthumb -mtp=soft -munaligned-access
CFLAGS			+= $(CINC)
CFLAGS			+= -DNRF9160_XXAA

ASMFILES		:= ./jump.S
ASMFILES		+= 

LDFLAGS			:= -T nrf9160.ld -flto -ffreestanding -nostdlib
#LDFLAGS			+= -L ./nrfxlib/bsdlib/lib/cortex-m33/soft-float/ -lbsd_nrf9160_xxaa
#LDFLAGS			+= -L ./nrfxlib/crypto/nrf_oberon/lib/cortex-m33/soft-float/ -loberon_3.0.2

OBJ       		= $(ASMFILES:.S=.o) $(CFILES:.c=.o) $(EXTC:.c=.o) $(CPPFILES:.cpp=.o) $(EXTCPP:.cpp=.o)


all: $(PRJ).elf


clean:
	rm -f *.hex *.elf *.o

# objects from asm files
.s.o:
	$(CC) -c $(CFLAGS) $< -o $@

# objects from c files
.c.o:
	$(CC) -c $(CFLAGS) $< -o $@


# elf file
$(PRJ).elf: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) --output $@ $(LDFLAGS)
