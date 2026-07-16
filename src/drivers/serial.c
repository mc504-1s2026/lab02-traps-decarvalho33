#include <kernel/serial.h>
#include <kernel/panic.h>
#include <arch/plic.h>
#include <arch/spinlock.h>
#include <arch/csr.h>

#define SERIAL_BUF_SIZE 1024

struct serial_dev {
    char buf[SERIAL_BUF_SIZE];
    size_t len;
    struct spinlock lock;
} dev = {0};

static inline u8 serial_read_reg(u64 offset)
{
    return *(volatile u8 *)(SERIAL_BASE + offset);
}

static inline void serial_write_reg(u64 offset, u8 val)
{
    *(volatile u8 *)(SERIAL_BASE + offset) = val;
}

void serial_init()
{
    /* desabilita  */
    serial_write_reg(SERIAL_IER, 0x00);

    /* 8 bitsword len */
    serial_write_reg(SERIAL_LCR, 0x03);

    /* FIFO pra RX TX e LIMPAA */
    serial_write_reg(SERIAL_FCR, SERIAL_FCR_FIFO_ENABLE | SERIAL_FCR_RX_FIFO_CLEAR | SERIAL_FCR_TX_FIFO_CLEAR);
    serial_write_reg(SERIAL_IER, SERIAL_IER_ERBFI);
}

void serial_irq_enable()
{
    plic_irq_set_priority(IRQ_SERIAL, 1);
    plic_hart_set_threshold(0, 0);
    plic_hart_enable_irq(0, IRQ_SERIAL);

    csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{

    plic_irq_set_priority(IRQ_SERIAL, 0);
    csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
    u32 irq = plic_hart_claim_irq(0);
    
    if (irq == IRQ_SERIAL) {
        spin_lock(&dev.lock);
        /*  buffer */
        while (serial_read_reg(SERIAL_LSR) & SERIAL_LSR_DTR) {
            char c = serial_read_reg(SERIAL_RBR);
            if (dev.len < SERIAL_BUF_SIZE) {
                dev.buf[dev.len++] = c;
            }
        }
        spin_unlock(&dev.lock);
    }
    
    if (irq) {
        plic_hart_complete_irq(0, irq);
    }
}

size_t serial_read(char *buf)
{
    u64 flags = spin_lock_irqsave(&dev.lock);
    size_t size = dev.len;
    for (size_t i = 0; i < size; i++) {
        buf[i] = dev.buf[i];
    }
    dev.len = 0;
    spin_unlock_irqrestore(&dev.lock, flags);

    return size;
}

void serial_putc(char c)
{
    while ((serial_read_reg(SERIAL_LSR) & SERIAL_LSR_THRE) == 0) { }
    serial_write_reg(SERIAL_THR, c);
}

void serial_puts(char *str)
{
    while (*str) {
        serial_putc(*str++);
    }
}
