CC = i686-elf-gcc
AS = i686-elf-as
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib

# Get compiler-provide crts
CRTBEGIN_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)

# Src files
ASM_SOURCES = src/boot.s src/crti.s src/crtn.s
C_SOURCES = src/kernel.c

# OBJ files
CRTI_OBJ = crti.o
CRTN_OBJ = crtn.o
KERNEL_OBJS = boot.o kernel.o

OBJ_LINK_LIST = $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(KERNEL_OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)

.PHONY: all clean run iso

all: myos.bin

# Assemble
boot.o: src/boot.s
	$(AS) $< -o $@

crti.o: src/crti.s
	$(AS) $< -o $@

crtn.o: src/crtn.s
	$(AS) $< -o $@

# Compile .c files
kernel.o: src/kernel.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Link everything in the correct order
myos.bin: $(CRTI_OBJ) $(KERNEL_OBJS) $(CRTN_OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ_LINK_LIST) -lgcc
	grub-file --is-x86-multiboot myos.bin

# Build ISO
iso: myos.bin
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir

# Build and run in QEMU
run: iso
	qemu-system-i386 -cdrom myos.iso

clean:
	rm -f *.o myos.bin myos.iso
	rm -rf isodir