.code64
.section .multiboot
header_start:
	.long 0xe85250d6
	.long 0
	.long header_end - header_start
	.long 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
	.word 0x00
	.word 0x00
	.long 0x08
header_end:

.code32
.section .bootstrap_stack, "aw", @nobits
  stack_bottom:
  .skip 16384
  stack_top:

.section .text
.extern init
.type init, @function
.global _start
.type _start, @function
_start:

    mov $stack_top, %esp

    and $-16, %esp
    pushl %esp
    pushl $0
    popf

    pushl %esp # arg[3]: Initial Kernel ESP
    pushl %ebx # arg[2]: MBI Kernel Struct
    pushL %eax # arg[1]: Multiboot Magic Number

    cli
    call init
    jmp hang

hang:
    hlt
    jmp hang

.section .x86_kend
.global x86_kernel_end
x86_kernel_end:
