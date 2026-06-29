#include <arch/csr.h>
#include <arch/timer.h>
#include <kernel/serial.h>

static bool alarm_armed;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_STIE);
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 now = timer_read();
	u64 when = now + secs * TIMER_FREQ;

	alarm_armed = true;
	csr_write(CSR_STIMECMP, when);
}

void timer_irq()
{
	if (!alarm_armed)
		return;

	alarm_armed = false;
	csr_write(CSR_STIMECMP, ~0UL);
	serial_puts("alarm\r\n> ");
}
