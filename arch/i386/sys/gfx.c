#include <i386/gfx.h>
#include <i386/vga.h>
#include <string.h>

unsigned char g_80x25_text[] =
{
/* MISC */
    0x67,
/* SEQ */
    0x03, 0x00, 0x03, 0x00, 0x02,
/* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
    0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
    0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
    0xFF,
/* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
    0xFF,
/* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x0C, 0x00, 0x0F, 0x08, 0x00
};

unsigned char g_320x200x256[] =
{
/* MISC */
    0x63,
/* SEQ */
    0x03, 0x01, 0x0F, 0x00, 0x0E,
/* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF,
/* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF,
/* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};

int currmode = 0;

void update_mode_info(int mode)
{
    switch(mode)
    {
        case(TEXT_80x25):
            currmode = TEXT_80x25; // 80x25 Text Mode
            break;
        case(SD_320x200x256):
            currmode = SD_320x200x256; // 320x200x256 SD Mode
        default:
            break;
    }
}

void init_text_mode()
{
    vbe_write_regs(g_80x25_text);
    uint8_t* vga_poke = (uint8_t*) 0x400;
    vga_poke[0x4A] = 0x00;
    vga_poke[0x4A+1] = 80;
    vga_poke[0x4C] = 0x0f;
    vga_poke[0x4C+1] = 0xa0;
    vga_poke[0x50] = 0x00;
    vga_poke[0x50+1] = 0x00;
    vga_poke[0x60] = 15;
    vga_poke[0x61] = 14;
    vga_poke[0x84] = 24;
    vga_poke[0x85] = 16;

    /* cakeh: We still need to clear screen and
       finish up last things */
    setup_text_mode();
}

void init_vbe(char *regs)
{
    if (!strcmp(regs, "modes")) {
        /* Make mode gathering automatic
           either by using multiboot header
           or using GPU and Monitor values
           or maybe EDID? */
        printk("Available VESA modes:\n");
        printk("Height     Width     BPP\n");
        printk("320     x  200    x  256\n");
        printk("80      x  25\n");
    } else if (!strcmp(regs, "320x200x256")) {
        if (currmode == SD_320x200x256) {
            printkc("You can't set a mode you're already using!\n", RED);
        } else {
            vbe_write_regs(g_320x200x256);
            vbe_clear_screen(9);
            vbe_draw_rectangle(0,0,320,10,8); // Title Bar
            vbe_printk("cobalt vbe test");
            update_mode_info(SD_320x200x256);
        }
    } else if (!strcmp(regs, "80x25")) {
        if (currmode == TEXT_80x25) {
            printkc("You can't set a mode you're already using!\n", RED);
        } else {
            vbe_write_regs(g_80x25_text);
        }
    } else if (!strcmp(regs, "") || !strcmp(regs, "help")) {
        printkc("Usage:\n", LBLUE);
        printk("vbe MODE\n");
        printkc("For available modes, use:\n", LBLUE);
        printk("vbe modes\n");
        printkc("Example:\n", LBLUE);
        printk("vbe 320x200x256\n");
    } else {
        printkc("VESA mode not found!\n", RED);
    }
}