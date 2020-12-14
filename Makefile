KERNEL=kernel.elf
IMAGE=kernel.iso
IMAGEDIR = iso
BINDIR = ./bin
INCDIR= -Iinclude

include ./include/cobalt/arch.mk

CFLAG:=$(CFLAG) -D$(ARCH)

kernel: 
	make clean
	make compile
	make iso
	
all:
	make clean
	make compile
	make iso
	make run
	
debug:
	make clean
	make compile
	make iso
	make run
	make clean

compile: $(OBJS)
	@echo "LD   $(KERNEL)"
ifeq (${ARCH},i386)
	i786-elf-gcc ${LDFLAG}
else
	gcc ${LDFLAG}
endif
	mv ${OBJS} ${BINDIR}/objs
	mv ${KERNEL} ${BINDIR}

%.o: %.c
	@echo "CC   $<"
ifeq (${ARCH},i386)
	i786-elf-gcc ${CFLAG} ${INCDIR} -c $< -o $@
else
	gcc ${CFLAG} ${INCDIR} -c $< -o $@
endif

%.o: %.s
	@echo "AS   $<"
ifeq (${ARCH},i386)
	i786-elf-as $< -o $@
else
	as $< -o $@
endif

%.o: %.S
	@echo "AS   $<"
ifeq (${ARCH},i386)
	i786-elf-as $< -o $@
else
	as $< -o $@
endif

%.o: %.asm
	@echo "AS   $<"

ifeq (${ARCH},i386)
	nasm -felf32 $< -o $@
else
	nasm -fbin $< -o $@
endif

clean:
	@echo "RM    OBJS"
	rm -rf ${OBJS}
	rm -rf ${BINDIR}/objs/*.o
	@echo "RM    BIN"
	rm -rf ${BINDIR}/${KERNEL}
	rm -rf ${IMAGEDIR}/boot/${KERNEL}
	@echo "RM    ISO"
	rm -rf *.iso
	rm -rf iso
	@echo "RM    INITRD"
	rm -rf make_initrd
	rm -rf initrd.img
	rm -rf *.txt
iso:
	-@mkdir -p iso/boot/grub
	-@echo $(CPUARCH) > iso/boot/grub/grub.cfg
	-@cp ${BINDIR}/${KERNEL} iso/boot/${KERNEL}
	-@gcc tools/make_initrd.c -o make_initrd
	-@printf "Cobalt version: 1.0\0" > version.txt
	-@printf "Welcome to Cobalt!\0" > motd.txt
	-@./make_initrd version.txt version.txt motd.txt motd.txt
	-@mv initrd.img iso/boot/initrd.img
	-@echo "ISO  ${IMAGE}"
	-@grub-mkrescue -o ${IMAGE} ${IMAGEDIR} --grub2-boot-info 2>/dev/null || true

run:
	qemu-system-${ARCH} -cdrom kernel.iso

$(V).SILENT:
