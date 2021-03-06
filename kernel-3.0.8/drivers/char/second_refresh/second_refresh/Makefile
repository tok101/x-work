TOPDIR = .

CC = mips-linux-gnu-gcc
LD = mips-linux-gnu-ld
OBJCOPY = mips-linux-gnu-objcopy
OBJDUMP	= mips-linux-gnu-objdump
drop-sections := .reginfo .mdebug .oomment .note .pdr .options .MIPS.options
strip-flags   := $(addprefix --remove-section=,$(drop-sections))

CFLAGS += -I$(TOPDIR)/include
CFLAGS += -nostdinc -Wall -Wundef -Werror-implicit-function-declaration \
	 -fno-common -EL -Os -march=mips32 -mabi=32 -G 0 -mno-abicalls -fno-pic\
	 -msoft-float

LDFLAGS	:= -nostdlib -EL -T target.ld
OBJCOPY_ARGS  := -O elf32-tradlittlemips

OBJS := $(TOPDIR)/src/start.o		\
	$(TOPDIR)/src/interface.o		\
	$(TOPDIR)/src/xxx_ops.o		\


all: $(TOPDIR)/firmware.bin
	@hexdump -v -e '"0x" 1/4 "%08x" "," "\n"' $< > $(TOPDIR)/firmware.hex

$(TOPDIR)/firmware.bin:$(TOPDIR)/firmware.o
	@$(LD) -nostdlib -EL -T $(TOPDIR)/target.ld $(OBJS) -Map $(TOPDIR)/tmp.map -o $(TOPDIR)/tmp.elf
	@$(OBJCOPY) $(strip-flags) $(OBJCOPY_ARGS) -O binary $(TOPDIR)/tmp.elf $@
	@$(OBJDUMP) $(TOPDIR)/tmp.elf -D > tmp.dump
$(TOPDIR)/firmware.o:$(OBJS)

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $^

%.o:%.S
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	find . -name "*.o" | xargs rm -vf
	find . -name "*.o.cmd" | xargs rm -vf
	find . -name "*.hex" | xargs rm -vf
	find . -name "*.bin" | xargs rm -vf
	find . -name "*.dump" | xargs rm -vf
	find . -name "*.tlb" | xargs rm -vf
	find . -name "*.map" | xargs rm -vf
	find . -name "*.elf" | xargs rm -vf

