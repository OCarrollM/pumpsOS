# Makefile — fixed: compile with -c, single link step
CC = gcc
ASM = gcc          # use gcc to assemble GAS-style .S files
CFLAGS = -ffreestanding -O2 -Wall -Wextra -nostdlib -nostartfiles -fno-builtin
BUILD = build
SRC = src
ISO = $(BUILD)/iso

.PHONY: all run clean

all: $(BUILD)/kernel.elf iso

$(BUILD):
	mkdir -p $(BUILD)

# compile C kernel -> object
$(BUILD)/kernel.o: $(SRC)/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# assemble entry.S -> object (use -c to avoid linking)
$(BUILD)/entry.o: $(SRC)/entry.S | $(BUILD)
	$(CC) -c $< -o $@

# single final link step: link both objects into kernel.elf
# -Ttext 0x100000 places the code at 1 MiB which QEMU -kernel can load
$(BUILD)/kernel.elf: $(BUILD)/entry.o $(BUILD)/kernel.o | $(BUILD)
	$(CC) -nostdlib -nostartfiles -Ttext 0x100000 -o $@ $^

iso: $(BUILD)/kernel.elf
	mkdir -p $(ISO)/boot/grub
	cp $(BUILD)/kernel.elf $(ISO)/boot/
	@echo 'set timeout=0' > $(ISO)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISO)/boot/grub/grub.cfg
	@echo '' >> $(ISO)/boot/grub/grub.cfg
	@echo 'menuentry "My Pumps Kernel" {' >> $(ISO)/boot/grub/grub.cfg
	@echo '    multiboot2 /boot/kernel.elf' >> $(ISO)/boot/grub/grub.cfg
	@echo '    boot' >> $(ISO)/boot/grub/grub.cfg
	@echo '}' >> $(ISO)/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/kernel.iso $(ISO)

clean:
	rm -rf $(BUILD)

run: all
	qemu-system-x86_64 -cdrom $(BUILD)/kernel.elf -serial stdio 

