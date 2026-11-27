CXX = g++
AS = nasm
LD = ld

CXXFLAGS = -m32 -ffreestanding -fno-threadsafe-statics -O2 -Wall -Wextra -fno-exceptions -fno-rtti -std=c++20 -Iinclude -Iinclude/core -Iinclude/graphics -Iinclude/graphics/games -Iinclude/graphics/app
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

QEMU = "/mnt/c/Program Files/qemu/qemu-system-x86_64.exe"
BUILD_DIR = build
SRC_CORE = src/core
SRC_GFX = src/graphics
SRC_GAME = $(SRC_GFX)/games
SRC_APP = $(SRC_GFX)/app
INC_CORE = include/core
INC_GFX = include/graphics
INC_GAME = $(INC_GFX)/games
INC_APP = $(INC_GFX)/app
INITRD_DIR = initrd_root
INITRD_TAR = $(BUILD_DIR)/initrd.tar
ISO_PATH = $(BUILD_DIR)/os.iso
WIN_ISO_PATH = C:/temp/singularity.iso

OBJS = \
	$(BUILD_DIR)/boot.o \
	$(BUILD_DIR)/switch.o \
	$(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/vga.o \
	$(BUILD_DIR)/gdt_flush.o \
	$(BUILD_DIR)/gdt.o \
	$(BUILD_DIR)/idt_load.o \
	$(BUILD_DIR)/idt.o \
	$(BUILD_DIR)/pic.o \
	$(BUILD_DIR)/keyboard.o \
	$(BUILD_DIR)/mouse.o \
	$(BUILD_DIR)/pmm.o \
	$(BUILD_DIR)/memory.o \
	$(BUILD_DIR)/heap.o \
	$(BUILD_DIR)/serial.o \
	$(BUILD_DIR)/timer.o \
	$(BUILD_DIR)/shell.o \
	$(BUILD_DIR)/paging.o \
	$(BUILD_DIR)/tss.o \
	$(BUILD_DIR)/task.o \
	$(BUILD_DIR)/initrd.o \
	$(BUILD_DIR)/framebuffer.o \
	$(BUILD_DIR)/bmp.o \
	$(BUILD_DIR)/_window.o \
	$(BUILD_DIR)/taskbar.o \
	$(BUILD_DIR)/text_editor.o \
	$(BUILD_DIR)/calculator.o

all: $(ISO_PATH)

$(BUILD_DIR)/boot.o: boot.asm
	mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) boot.asm -o $@

$(BUILD_DIR)/switch.o: $(SRC_CORE)/switch.asm
	mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $(SRC_CORE)/switch.asm -o $@

$(BUILD_DIR)/kernel.o: kernel.cpp $(INC_CORE)/vga.hpp $(INC_CORE)/gdt.hpp $(INC_CORE)/idt.hpp $(INC_CORE)/pic.hpp $(INC_CORE)/keyboard.hpp $(INC_CORE)/mouse.hpp $(INC_CORE)/cursor.hpp $(INC_GFX)/icons.hpp $(INC_GFX)/_window.hpp $(INC_GFX)/taskbar.hpp $(INC_APP)/text_editor.hpp $(INC_APP)/calculator.hpp $(INC_CORE)/multiboot.hpp $(INC_CORE)/pmm.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/heap.hpp $(INC_CORE)/serial.hpp $(INC_CORE)/timer.hpp $(INC_CORE)/shell.hpp $(INC_CORE)/paging.hpp $(INC_CORE)/tss.hpp $(INC_CORE)/task.hpp $(INC_CORE)/initrd.hpp $(INC_CORE)/framebuffer.hpp $(INC_CORE)/bmp.hpp $(INC_CORE)/rtc.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c kernel.cpp -o $@

$(BUILD_DIR)/vga.o: $(SRC_CORE)/vga.cpp $(INC_CORE)/vga.hpp $(INC_CORE)/io.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/vga.cpp -o $@

$(BUILD_DIR)/gdt_flush.o: $(SRC_CORE)/gdt_flush.asm
	mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $(SRC_CORE)/gdt_flush.asm -o $@

$(BUILD_DIR)/gdt.o: $(SRC_CORE)/gdt.cpp $(INC_CORE)/gdt.hpp $(INC_CORE)/tss.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/gdt.cpp -o $@

$(BUILD_DIR)/idt_load.o: $(SRC_CORE)/idt_load.asm
	mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $(SRC_CORE)/idt_load.asm -o $@

$(BUILD_DIR)/idt.o: $(SRC_CORE)/idt.cpp $(INC_CORE)/idt.hpp $(INC_CORE)/pic.hpp $(INC_CORE)/vga.hpp $(INC_CORE)/serial.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/idt.cpp -o $@

$(BUILD_DIR)/pic.o: $(SRC_CORE)/pic.cpp $(INC_CORE)/pic.hpp $(INC_CORE)/io.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/pic.cpp -o $@

$(BUILD_DIR)/keyboard.o: $(SRC_CORE)/keyboard.cpp $(INC_CORE)/keyboard.hpp $(INC_CORE)/pic.hpp $(INC_CORE)/io.hpp $(INC_CORE)/vga.hpp $(INC_CORE)/shell.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/keyboard.cpp -o $@

$(BUILD_DIR)/mouse.o: $(SRC_CORE)/mouse.cpp $(INC_CORE)/mouse.hpp $(INC_CORE)/idt.hpp $(INC_CORE)/pic.hpp $(INC_CORE)/io.hpp $(INC_CORE)/serial.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/mouse.cpp -o $@

$(BUILD_DIR)/pmm.o: $(SRC_CORE)/pmm.cpp $(INC_CORE)/pmm.hpp $(INC_CORE)/multiboot.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/pmm.cpp -o $@

$(BUILD_DIR)/memory.o: $(SRC_CORE)/memory.cpp $(INC_CORE)/memory.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/memory.cpp -o $@

$(BUILD_DIR)/heap.o: $(SRC_CORE)/heap.cpp $(INC_CORE)/heap.hpp $(INC_CORE)/pmm.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/paging.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/heap.cpp -o $@

$(BUILD_DIR)/serial.o: $(SRC_CORE)/serial.cpp $(INC_CORE)/serial.hpp $(INC_CORE)/io.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/serial.cpp -o $@

$(BUILD_DIR)/timer.o: $(SRC_CORE)/timer.cpp $(INC_CORE)/timer.hpp $(INC_CORE)/pic.hpp $(INC_CORE)/io.hpp $(INC_CORE)/task.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/timer.cpp -o $@

$(BUILD_DIR)/shell.o: $(SRC_CORE)/shell.cpp $(INC_CORE)/shell.hpp $(INC_CORE)/vga.hpp $(INC_CORE)/timer.hpp $(INC_CORE)/pmm.hpp $(INC_CORE)/heap.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/serial.hpp $(INC_CORE)/task.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/shell.cpp -o $@

$(BUILD_DIR)/paging.o: $(SRC_CORE)/paging.cpp $(INC_CORE)/paging.hpp $(INC_CORE)/pmm.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/serial.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/paging.cpp -o $@

$(BUILD_DIR)/tss.o: $(SRC_CORE)/tss.cpp $(INC_CORE)/tss.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/serial.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/tss.cpp -o $@

$(BUILD_DIR)/task.o: $(SRC_CORE)/task.cpp $(INC_CORE)/task.hpp $(INC_CORE)/heap.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/serial.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/task.cpp -o $@

$(BUILD_DIR)/initrd.o: $(SRC_CORE)/initrd.cpp $(INC_CORE)/initrd.hpp $(INC_CORE)/multiboot.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/serial.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/initrd.cpp -o $@

$(BUILD_DIR)/framebuffer.o: $(SRC_CORE)/framebuffer.cpp $(INC_CORE)/framebuffer.hpp $(INC_CORE)/multiboot.hpp $(INC_CORE)/paging.hpp $(INC_CORE)/pmm.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/serial.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/framebuffer.cpp -o $@

$(BUILD_DIR)/bmp.o: $(SRC_CORE)/bmp.cpp $(INC_CORE)/bmp.hpp $(INC_CORE)/initrd.hpp $(INC_CORE)/framebuffer.hpp $(INC_CORE)/paging.hpp $(INC_CORE)/pmm.hpp $(INC_CORE)/memory.hpp $(INC_CORE)/serial.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_CORE)/bmp.cpp -o $@

$(BUILD_DIR)/_window.o: $(SRC_GFX)/_window.cpp $(INC_GFX)/_window.hpp $(INC_APP)/text_editor.hpp $(INC_APP)/calculator.hpp $(INC_CORE)/framebuffer.hpp $(INC_CORE)/initrd.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_GFX)/_window.cpp -o $@

$(BUILD_DIR)/taskbar.o: $(SRC_GFX)/taskbar.cpp $(INC_GFX)/taskbar.hpp $(INC_CORE)/framebuffer.hpp $(INC_CORE)/mouse.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_GFX)/taskbar.cpp -o $@

$(BUILD_DIR)/text_editor.o: $(SRC_APP)/text_editor.cpp $(INC_APP)/text_editor.hpp $(INC_CORE)/initrd.hpp $(INC_CORE)/framebuffer.hpp $(INC_CORE)/font.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_APP)/text_editor.cpp -o $@

$(BUILD_DIR)/calculator.o: $(SRC_APP)/calculator.cpp $(INC_APP)/calculator.hpp $(INC_CORE)/framebuffer.hpp $(INC_CORE)/font.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_APP)/calculator.cpp -o $@

$(INITRD_TAR): $(wildcard $(INITRD_DIR)/*)
	mkdir -p $(BUILD_DIR) $(INITRD_DIR)
	tar --format=ustar -cf $@ -C $(INITRD_DIR) .

$(BUILD_DIR)/kernel.bin: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(ISO_PATH): $(BUILD_DIR)/kernel.bin $(INITRD_TAR)
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/isodir/boot/kernel.bin
	cp $(INITRD_TAR) $(BUILD_DIR)/isodir/boot/initrd.tar
	printf 'set timeout=0\nset default=0\nmenuentry "Singularity" {\n\tmultiboot /boot/kernel.bin\n\tmodule /boot/initrd.tar\n\tboot\n}\n' > $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir

run: $(ISO_PATH)
	mkdir -p /mnt/c/temp
	cp $(ISO_PATH) /mnt/c/temp/singularity.iso
	$(QEMU) -cdrom $(WIN_ISO_PATH) -serial stdio

clean:
	rm -rf $(BUILD_DIR) /mnt/c/temp/singularity.iso