#include <stdint.h>

extern void tss_flush();

void set_kernel_stack(uint32_t stack);
void write_tss(int32_t num, uint16_t ss0, uint32_t esp0);