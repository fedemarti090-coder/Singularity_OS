CXX = g++
AS = nasm
LD = ld

CXXFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -std=c++20
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

QEMU = "/mnt/c/Program Files/qemu/qemu-system-x86_64.exe"
BUILD_DIR = build
ISO_PATH = $(BUILD_DIR)/os.iso
WIN_ISO_PATH = C:/temp/singularity.iso

all: $(ISO_PATH)

$(BUILD_DIR)/boot.o: boot.asm
	mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) boot.asm -o $@

$(BUILD_DIR)/kernel.o: kernel.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c kernel.cpp -o $@

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o linker.ld
	$(LD) $(LDFLAGS) $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o -o $@

$(ISO_PATH): $(BUILD_DIR)/kernel.bin
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/isodir/boot/kernel.bin
	printf 'set timeout=0\nset default=0\nmenuentry "Singularity" {\n\tmultiboot /boot/kernel.bin\n\tboot\n}\n' > $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir

run: $(ISO_PATH)
	mkdir -p /mnt/c/temp
	cp $(ISO_PATH) /mnt/c/temp/singularity.iso
	$(QEMU) -drive file=$(WIN_ISO_PATH),format=raw,media=cdrom

clean:
	rm -rf $(BUILD_DIR) /mnt/c/temp/singularity.iso