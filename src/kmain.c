#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];

#define CMD_BUF_SIZE 256

void kmain()
{
    printk_set_level(LOG_DEBUG);
    info("entered S-mode\n");
    info("booting on hart %d\n", _hartid[0]);
    info("setting up virtual memory...\n");
    vm_init();

    info("enabling traps...\n");
    trap_setup();
    info("enabling timer...\n");
    timer_irq_enable();
    info("enabling serial...\n");
    serial_init();
    serial_irq_enable();
    hart_irq_enable();

    char cmd_buf[CMD_BUF_SIZE];
    size_t cmd_len = 0;
    char rx_buf[64];

    serial_puts("> ");

    while (1) {
        size_t n = serial_read(rx_buf);
        
        for (size_t i = 0; i < n; i++) {
            char c = rx_buf[i];
            
            if (c == '\r' || c == '\n') {
                serial_puts("\n");
                cmd_buf[cmd_len] = '\0';
                
                if (cmd_len > 0) {
                    if (strncmp(cmd_buf, "uptime", 6) == 0) {
                        u64 t = timer_read() / TIMER_FREQ;
                        printk(LOG_INFO, "%lus\n", t);
                    } else if (strncmp(cmd_buf, "echo ", 5) == 0) {
                        printk(LOG_INFO, "%s\n", cmd_buf + 5);
                    } else if (strncmp(cmd_buf, "alarm ", 6) == 0) {
                        u64 delay = strtou64(cmd_buf + 6, 10);
                        timer_set_alarm(delay);
                    } else if (strncmp(cmd_buf, "echo", 4) == 0) {
                        printk(LOG_INFO, "\n");
                    } else {
                        printk(LOG_INFO, "command not found\n");
                    }
                }
                
                cmd_len = 0;
                serial_puts("> ");
                
            } else if (c == '\b' || c == 0x7F) {
                /* cosmetico */
                if (cmd_len > 0) {
                    cmd_len--;
                    serial_puts("\b \b");
                }
            } else {
                if (cmd_len < CMD_BUF_SIZE - 1) {
                    cmd_buf[cmd_len++] = c;
                    /* echo c */
                    serial_putc(c); 
                }
            }
        }
    }
}
