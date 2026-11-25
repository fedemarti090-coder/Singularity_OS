CXX = g++
AS = nasm
LD = ld

CXXFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -std=c++20 -Iinclude
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

$(BUILD_DIR)/kernel.o: kernel.cpp include/vga.hpp include/gdt.hpp include/idt.hpp include/pic.hpp include/keyboard.hpp include/multiboot.hpp include/pmm.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c kernel.cpp -o $@

$(BUILD_DIR)/vga.o: src/vga.cpp include/vga.hpp include/io.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/vga.cpp -o $@

$(BUILD_DIR)/gdt_flush.o: src/gdt_flush.asm
	mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) src/gdt_flush.asm -o $@

$(BUILD_DIR)/gdt.o: src/gdt.cpp include/gdt.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/gdt.cpp -o $@

$(BUILD_DIR)/idt_load.o: src/idt_load.asm
	mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) src/idt_load.asm -o $@

$(BUILD_DIR)/idt.o: src/idt.cpp include/idt.hpp include/pic.hpp include/vga.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/idt.cpp -o $@

$(BUILD_DIR)/pic.o: src/pic.cpp include/pic.hpp include/io.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/pic.cpp -o $@

$(BUILD_DIR)/keyboard.o: src/keyboard.cpp include/keyboard.hpp include/pic.hpp include/io.hpp include/vga.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/keyboard.cpp -o $@

$(BUILD_DIR)/pmm.o: src/pmm.cpp include/pmm.hpp include/multiboot.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/pmm.cpp -o $@

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/vga.o $(BUILD_DIR)/gdt_flush.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/idt_load.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/pic.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/pmm.o linker.ld
	$(LD) $(LDFLAGS) $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/vga.o $(BUILD_DIR)/gdt_flush.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/idt_load.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/pic.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/pmm.o -o $@

$(ISO_PATH): $(BUILD_DIR)/kernel.bin
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/isodir/boot/kernel.bin
	printf 'set timeout=0\nset default=0\nmenuentry "Singularity" {\n\tmultiboot /boot/kernel.bin\n\tboot\n}\n' > $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir

run: $(ISO_PATH)
	mkdir -p /mnt/c/temp
	cp $(ISO_PATH) /mnt/c/temp/singularity.iso
	$(QEMU) -cdrom $(WIN_ISO_PATH)

clean:
	rm -rf $(BUILD_DIR) /mnt/c/temp/singularity.iso