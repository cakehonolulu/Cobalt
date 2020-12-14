#include <i386/vga.h>
#include <i386/kheap.h>
#include <i386/gfx.h>
#include <i386/8042.h>
#include <i386/panic.h>
#include <multiboot.h>
#include <string.h>
#include <stdint.h>

char *commands[255]; //Should be plenty, at least for now.
void (*cmd_handlers[255])(char *);
uint8_t cmd_index = 0;
char cmd_history[50][512];
uint8_t cmd_his_len = 0;
char key_buffer[512];
void (*ext_handler)(char key);
void (*special_handler)(enum SPECIAL key);
uint8_t cmd_his_index = 0;

int parse_command(char *linein) {
  for(int i = 0; i < cmd_index; i++) {
    char cmd[256] = "";
    char args[256] = "";
    strsplit(linein, ' ', cmd, args);
    tolowercase(cmd);
    if(strcmp(cmd, commands[i]) == 0) {
      cmd_handlers[i](args);
      return 1;
    }
  }
  return 0;
}

void delete_last()
{
  update_cursor(get_row()-1, get_col());
  putch(' ');
  update_cursor(get_row()-1, get_col());
}

void backspace(char s[]) {
  int len = strlen(s);
  s[len-1] = '\0';
}

void register_keyboard_handler(void (*handler)(char key), void(*special)(enum SPECIAL key))
{
  ext_handler = handler;
  special_handler = special;
}

void remove_keyboard_handler()
{
  ext_handler = 0;
  special_handler = 0;
}

void keyboard_handler(char key)
{
  char printstr[] = {key, 0};
  printk(printstr);
  if(!ext_handler)
  {
    append(key_buffer, key);
  }
  else
  {
    ext_handler(key);
  }
}

void goto_cmd_history()
{
  for (int i = 0; i < strlen(key_buffer); i++)
  {
    //delete_last();
  }
  strcpy(key_buffer, cmd_history[cmd_his_index]);
  printk(cmd_history[cmd_his_index]);
}

void keyboard_handler_special(enum SPECIAL key) {
  //Note: we check for ext_handler, not special_handler, because we don't want the terminal
  //to still act like a terminal when a cmd program is running.
  if(!ext_handler)
  {
    if (key == BACKSPACE)
    {
      if (strlen(key_buffer) > 0)
      {
        backspace(key_buffer);
        delete_last();
      }
    }
    else if (key == ENTER)
    {
      cmd_his_index = 0;
      //Shift the cmd_history array down
      for (int i = 48; i >= 1; i--)
      {
        strcpy(cmd_history[i + 1], cmd_history[i]);
      }
      cmd_his_len++;
      strcpy(cmd_history[1], key_buffer);
      
      printk("\n");
      if(!parse_command(key_buffer))
      {
        printk("Command not found.\n");
      }
      key_buffer[0] = '\0';
      printk("$ ");
    }
    else if (key == ARW_UP)
    {
      if (cmd_his_index==0)
      {
        strcpy(cmd_history[0], key_buffer);
      }
      if (cmd_his_index < cmd_his_len)
      {
        cmd_his_index++;
        goto_cmd_history();
      }
    }
    else if (key == ARW_DOWN)
    {
      if (cmd_his_index > 0)
      {
        cmd_his_index--;
        goto_cmd_history();
      }
    }
  }
  else if(special_handler)
  {
    special_handler(key);
  }
}

void register_command(char *cmd, void (*handler)(char *)) {
  tolowercase(cmd);
  cmd_handlers[cmd_index] = handler;
  commands[cmd_index] = cmd;
  cmd_index++;
}

void help(char *args)
{
  UNUSED(args);
  printkc("Available commands:\n", LBLUE);
  printk("echo\nclear\npanic\nvbe\ncpuinfo\ndebug\nticks\npci\nmmap\n");
}

void echo(char *args)
{
  printk("%s\n", args);
}

void clear(char *args)
{
  UNUSED(args);
  cls();
}

void makepanic(char *args)
{
  if (!strcmp(args, "divisionbyzero"))
  {
    int a = 10;
    int b = 0;
    int c = a/b;
    printk("%d", c);
  } else if (!strcmp(args, "normal")) {
    PANIC("Called from shell!");
  } else if (!strcmp(args, "pagefault")) {
    unsigned int *ptr = (unsigned int*) 0xA0000000;
    unsigned int do_page_fault = *ptr;
    *ptr = NULL;
    do_page_fault = NULL;
    kmalloc(*ptr + do_page_fault);
  } else if (!strcmp(args, "help")) {
    printkc("Usage:\n", LBLUE);
    printk("panic MODE\n");
    printkc("Available modes:\n", LBLUE);
    printk("pagefault\n");
    printk("divisionbyzero\n");
    printk("normal\n");
  }
}

void vbe(char *args)
{
  x86_switch_screen_mode_command(args);
}

void cpuinfo(char *args)
{
  UNUSED(args);
  getCPUVendor();
  getCPUFeatures();
  getCPUArch();
  getCPUName();
}

void debug(char *args)
{
  UNUSED(args);
      extern uint32_t x86_memoryhead, x86_memoryend, x86_total_bytes, x86_free_bytes, x86_byte_allocations, x86_initial_esp, x86_kernel_end, x86_kernel_start, x86_initrd_size, x86_initrd_start, x86_initrd_end;
      extern uint32_t x86_ramsize, x86_kernel_size, x86_kernel_iss, x86_kernel_ise, x86_kernel_isi, x86_ramstart, x86_memory_end_location, x86_memory_amount, x86_memory_location, x86_usable_mem, x86_total_frames;
      printk("Total frames: %d\n", x86_total_frames);
      printk("Current Framebuffer: 0x%x\n", get_screen_fb());
      printk("Initial Stack Pointer: 0x%x\n", x86_initial_esp);
      printk("Kernel Base: 0x%x , Kernel End: 0x%x , Kernel Size: (%dKiB)\n", &x86_kernel_start, &x86_kernel_end, ((uint32_t) &x86_kernel_size) >> 10);
      printk("Boot Stack Base: 0x%x , Boot Stack End: 0x%x , Boot Stack Size: (%dKiB)\n", &x86_kernel_isi, &x86_kernel_ise, ((uint32_t) &x86_kernel_iss) >> 10);
      printk("Initrd start: 0x%x , Initrd end: 0x%x , Initrd Size: (", x86_initrd_start, x86_initrd_end);

      if(x86_initrd_size / 1024 / 1024 > 0)
      {
        printk("%d MiB)\n", x86_initrd_size / 1024 / 1024);
      }
      else if(x86_initrd_size / 1024 > 0)
      {
        printk("%d KiB)\n", x86_initrd_size / 1024);
      } else {
        printk("%d B)\n", x86_initrd_size);
      }

      printk("Heap Base: 0x%x , Heap End: 0x%x , Heap Size: (", x86_memoryhead, x86_memoryend);
      
      if(x86_total_bytes / 1024 / 1024 > 0)
      {
        printk("%d MiB)\nCurrent Heap Allocations: (%d)\n", x86_total_bytes / 1024 / 1024, x86_byte_allocations);
      }
      else if(x86_total_bytes / 1024 > 0)
      {
        printk("%d KiB)\nCurrent Heap Allocations: (%d)\n", x86_total_bytes / 1024, x86_byte_allocations);
      } else {
        printk("%d B)\nCurrent Heap Allocations: (%d)\n", x86_total_bytes, x86_byte_allocations);
      }

      if(x86_free_bytes / 1024 / 1024 > 0)
      {
        printk("Current Heap Free Memory: (%d MiB)\n", x86_free_bytes / 1024 / 1024);
      }
      else if(x86_free_bytes / 1024 > 0)
      {
        printk("Current Heap Free Memory: (%d KiB)\n", x86_free_bytes / 1024);
      } else {
        printk("Current Heap Free Memory: (%d B)\n", x86_free_bytes);
      }

        printk("Total System RAM: %d MB \n",  x86_ramsize);
        printk("Start of Physical RAM Memory    : 0x%x\n", (uint32_t) x86_ramstart);
        printk("End of Physical RAM Memory      : 0x%x\n", (uint32_t) x86_memory_end_location);
        printk("Psysical RAM Memory Amount Size : 0x%x\n", x86_memory_amount);
        printk("Usable RAM Memory Starts At     : 0x%x\n", (uint32_t) x86_memory_location);
        printk("Usable Amount of RAM Memory     : 0x%x\n", x86_usable_mem);
      
}

void ticks(char *args)
{
  UNUSED(args);
  printk("%d\n", get_ticks());
}

void pci(char *args)
{
  UNUSED(args);
  pci_scan();
}

void mmap(char *args)
{
  UNUSED(args);
  extern multiboot_info_t *mb;
  get_memory_map(mb);
}

void shell()
{
  register_command("help", help);
  register_command("echo", echo);
  register_command("clear", clear);
  register_command("panic", makepanic);
  register_command("vbe", vbe);
  register_command("cpuinfo", cpuinfo);
  register_command("debug", debug);
  register_command("ticks", ticks);
  register_command("pci", pci);
  register_command("mmap", mmap);
  printk("$ ");
  while(1);
}