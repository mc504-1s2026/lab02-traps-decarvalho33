#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <arch/csr.h>
#include <arch/timer.h>
#include <kernel/serial.h>

extern void trap_entry();

__asm__(
    ".balign 4\n"
    "trap_entry_aligned:\n"
    "    j trap_entry\n"
);
extern void trap_entry_aligned();

void handle_irq(u64 scause)
{
    if (scause == TRAP_TIMER_IRQ) {
        timer_irq();
    } else if (scause == TRAP_EXTERNAL_IRQ) {
        serial_irq();
    } else {
        panic("unexpected irq: %llx\n", scause);
    }
}

void handle_exception(u64 scause, u64 stval, u64 sepc)
{
    panic("unhandled exception: cause=%llx, stval=%llx, sepc=%llx\n", scause, stval, sepc);
}

void trap_setup()
{
    csr_write(CSR_STVEC, (u64)trap_entry_aligned);
}

void handle_trap()
{
    u64 scause = csr_read(CSR_SCAUSE);
    u64 stval = csr_read(CSR_STVAL);
    u64 sepc = csr_read(CSR_SEPC);

    if (scause & TRAP_IRQ_BIT) {
        handle_irq(scause);
    } else {
        handle_exception(scause, stval, sepc);
    }
}

void hart_irq_enable()
{
    csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
    u64 sstatus = csr_read(CSR_SSTATUS);
    csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
    return sstatus & CSR_SSTATUS_SIE;
}

void hart_irq_restore(u64 flags)
{
    if (flags) {
        csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
    } else {
        csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
    }
}

void hart_irq_disable()
{
    csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
