ifeq ($(ARCH), i386)
include ./arch/$(ARCH)/config.make
include ./arch/$(ARCH)/Makefile
CPUARCH = '\
set timeout=0\n\
set default=0\n\
menuentry "Cobalt" {\n\
\tmultiboot2 /boot/kernel.elf\n\
\tboot\n\
}'
endif