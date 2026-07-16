#include <arch/timer.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>

u64 timer_read()
{
    return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
    csr_set(CSR_SIE, CSR_SIE_STIE);
}

void timer_irq_disable()
{
    csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
    u64 now = timer_read();
    u64 tick_in_future = now + (secs * TIMER_FREQ);
    csr_write(CSR_STIMECMP, tick_in_future);
    timer_irq_enable();
}

void timer_irq()
{
    timer_irq_disable();
    csr_write(CSR_STIMECMP, -1ULL); 
    
    printk(LOG_INFO, "\nalarm\n> ");
}
