#include <arch/csr.h>
#include <arch/io.h>
#include <arch/plic.h>
#include <arch/spinlock.h>
#include <kernel/serial.h>

#define SERIAL_BUF_SIZE 256

static void *serial_reg(u64 offset)
{
	return (void *)((u64)SERIAL_BASE + offset);
}

static bool serial_has_data(void)
{
	return (ioread8(serial_reg(SERIAL_LSR)) & SERIAL_LSR_DTR) != 0;
}

struct serialdev {
	char buf[SERIAL_BUF_SIZE];
	size_t len;
	struct spinlock lock;
};
static struct serialdev dev;

void serial_init()
{
	spin_init(&dev.lock);
	dev.len = 0;

	iowrite8(0, serial_reg(SERIAL_IER));
	iowrite8(SERIAL_FCR_FIFO_ENABLE | SERIAL_FCR_RX_FIFO_CLEAR | SERIAL_FCR_TX_FIFO_CLEAR,
		 serial_reg(SERIAL_FCR));
	iowrite8(SERIAL_IER_ERBFI, serial_reg(SERIAL_IER));
}

void serial_irq_enable()
{
	plic_irq_set_priority(IRQ_SERIAL, 1);
	plic_hart_enable_irq(0, IRQ_SERIAL);
	plic_hart_set_threshold(0, 0);
	iowrite8(SERIAL_IER_ERBFI, serial_reg(SERIAL_IER));
	csr_set(CSR_SIE, CSR_SIE_SEIE);
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void serial_irq_disable()
{
	iowrite8(0, serial_reg(SERIAL_IER));
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	while (serial_has_data()) {
		u64 flags;
		char c = (char)ioread8(serial_reg(SERIAL_RBR));

		flags = spin_lock_irqsave(&dev.lock);
		if (dev.len < SERIAL_BUF_SIZE)
			dev.buf[dev.len++] = c;
		spin_unlock_irqrestore(&dev.lock, flags);

		if (c == '\r') {
			serial_putc('\r');
			serial_putc('\n');
		} else {
			serial_putc(c);
		}
	}
}

size_t serial_read(char *buf)
{
	u64 flags = spin_lock_irqsave(&dev.lock);
	size_t n = dev.len;

	for (size_t i = 0; i < n; i++)
		buf[i] = dev.buf[i];
	dev.len = 0;

	spin_unlock_irqrestore(&dev.lock, flags);
	return n;
}

void serial_puts(char *str)
{
	while (*str != '\0')
		serial_putc(*str++);
}

void serial_putc(char c)
{
	while ((ioread8(serial_reg(SERIAL_LSR)) & SERIAL_LSR_THRE) == 0) {
	}
	iowrite8((u8)c, serial_reg(SERIAL_THR));
}
