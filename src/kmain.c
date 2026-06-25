#include <kernel/printf.h>
#include <kernel/mm.h>
#include <kernel/string.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>

extern int _hartid[];

static void shell_prompt(void)
{
	serial_puts("> ");
}

static void shell_run_command(char *line)
{
	char buf[32];

	if (line[0] == '\0')
		return;

	if (strcmp(line, "uptime") == 0) {
		snprintf(buf, sizeof(buf), "%lus\r\n", timer_read() / TIMER_FREQ);
		serial_puts(buf);
		return;
	}

	if (strncmp(line, "echo ", 5) == 0) {
		serial_puts(line + 5);
		serial_puts("\r\n");
		return;
	}

	if (strncmp(line, "alarm ", 6) == 0) {
		timer_set_alarm(strtou64(line + 6, 10));
		return;
	}

	serial_puts("unknown command\r\n");
}

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

	char input[128];
	char rxbuf[64];
	size_t input_len = 0;

	shell_prompt();

	while (1) {
		size_t n = serial_read(rxbuf);

		for (size_t i = 0; i < n; i++) {
			char c = rxbuf[i];

			if (c == '\r') {
				input[input_len] = '\0';
				shell_run_command(input);
				input_len = 0;
				shell_prompt();
			} else if (c == '\n') {
				continue;
			} else if (c == '\b' || c == 0x7f) {
				if (input_len > 0)
					input_len--;
			} else if (input_len < sizeof(input) - 1) {
				input[input_len++] = c;
			}
		}
	}
}
