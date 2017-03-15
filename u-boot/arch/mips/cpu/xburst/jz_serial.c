/*
 * Jz4740 UART support
 * Copyright (c) 2011
 * Qi Hardware, Xiangfu Liu <xiangfu@sharism.cc>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <serial.h>
#include <linux/compiler.h>

#include <asm/io.h>
#include <asm/jz_uart.h>
#include <asm/arch/base.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SMALLER_SPL
unsigned char *spl_uart = (UART0_BASE + CONFIG_SYS_UART_INDEX * 0x1000);
void simple_puts(const char *s);

void simple_serial_init(void)
{
        unsigned int baud_div, tmp;

        /* Disable port interrupts while changing hardware */
        writeb(0, spl_uart + 0x4);

        /* Disable UART unit function */
        writeb(~UART_FCR_UUE, spl_uart + 0x8);

        /* Set both receiver and transmitter in UART mode (not SIR) */
        writeb(~(SIRCR_RSIRE | SIRCR_TSIRE), spl_uart + 0x20);

        /*
         * Set databits, stopbits and parity.
         * (8-bit data, 1 stopbit, no parity)
         */
        writeb(UART_LCR_WLEN_8 | UART_LCR_STOP_1, spl_uart + 0xc);

        /* Set baud rate */
        baud_div = CONFIG_SYS_EXTAL / 16 / CONFIG_BAUDRATE;
        tmp = readb(spl_uart + 0xc);
        tmp |= UART_LCR_DLAB;
        writeb(tmp, spl_uart + 0xc);
        writeb((baud_div >> 8) & 0xff, spl_uart + 0x4);
        writeb(baud_div & 0xff, spl_uart + 0);
        tmp &= ~UART_LCR_DLAB;
        writeb(tmp, spl_uart + 0xc);

        /* Enable UART unit, enable and clear FIFO */
        writeb(UART_FCR_UUE | UART_FCR_FE | UART_FCR_TFLS | UART_FCR_RFLS,
                spl_uart + 0x8);

	simple_puts("SPL\n");
}

void simple_putc(const char c)
{
        if (c == '\n')
                simple_putc('\r');
	writeb((u8)c, spl_uart + 0);

	while (!((readb(spl_uart + 0x14) & (UART_LSR_TDRQ | UART_LSR_TEMT)) == 0x60))
                ;
}

void simple_puts(const char *s)
{
        while(*s)
                simple_putc(*s++);
}

void simple_put_dec(unsigned int  d)
{
        char c[16];
        int i;
        int j = 0;
        int x = d;

        while (x /= 10)
                j++;

        for (i = j; i >= 0; i--) {
                c[i] = d % 10;
                c[i] += 0x30;
                d /= 10;
        }
        c[j + 1] = '\n';
        c[j + 2] = 0;
        simple_puts(c);
}
void simple_put_hex(unsigned int  d)
{
        unsigned char c[12];
        char i;
        for(i = 0; i < 8;i++)
        {
                c[i] = (d >> ((7 - i) * 4)) & 0xf;
                if(c[i] < 10)
                        c[i] += 0x30;
                else
                        c[i] += (0x41 - 10);
        }
        c[8] = '\n';
        c[9] = 0;
        simple_puts(c);

}
#endif

/*
 * serial_init - initialize a channel
 *
 * This routine initializes the number of data bits, parity
 * and set the selected baud rate. Interrupts are disabled.
 * Set the modem control signals if the option is selected.
 *
 * RETURNS: N/A
 */

struct jz_uart *uart __attribute__ ((section(".data")));

static int jz_serial_init(void)
{
#ifdef CONFIG_BURNER
	uart = (struct jz_uart *)(UART0_BASE + gd->arch.gi->uart_idx * 0x1000);
#else
	uart = (struct jz_uart *)(UART0_BASE + CONFIG_SYS_UART_INDEX * 0x1000);
#endif

	/* Disable port interrupts while changing hardware */
	writeb(0, &uart->dlhr_ier);

	/* Disable UART unit function */
	writeb(~UART_FCR_UUE, &uart->iir_fcr);

	/* Set both receiver and transmitter in UART mode (not SIR) */
	writeb(~(SIRCR_RSIRE | SIRCR_TSIRE), &uart->isr);

	/*
	 * Set databits, stopbits and parity.
	 * (8-bit data, 1 stopbit, no parity)
	 */
	writeb(UART_LCR_WLEN_8 | UART_LCR_STOP_1, &uart->lcr);

	/* Set baud rate */
	serial_setbrg();

	/* Enable UART unit, enable and clear FIFO */
	writeb(UART_FCR_UUE | UART_FCR_FE | UART_FCR_TFLS | UART_FCR_RFLS,
	       &uart->iir_fcr);

	return 0;
}

static void jz_serial_setbrg(void)
{
	u32 baud_div, tmp;
#ifdef CONFIG_BURNER
	baud_div = gd->arch.gi->extal / 16 / gd->arch.gi->baud_rate;
#else
	baud_div = CONFIG_SYS_EXTAL / 16 / CONFIG_BAUDRATE;
#endif

#ifdef CONFIG_PALLADIUM
	writel(32,0xb0030024);
	writel(0,0xb0030028);
	baud_div = 1;
#endif
	tmp = readb(&uart->lcr);
	tmp |= UART_LCR_DLAB;
	writeb(tmp, &uart->lcr);

	writeb((baud_div >> 8) & 0xff, &uart->dlhr_ier);
	writeb(baud_div & 0xff, &uart->rbr_thr_dllr);

	tmp &= ~UART_LCR_DLAB;
	writeb(tmp, &uart->lcr);
}

static int jz_serial_tstc(void)
{
	if (readb(&uart->lsr) & UART_LSR_DR)
		return 1;

	return 0;
}

static void jz_serial_putc(const char c)
{
	if (c == '\n')
		serial_putc('\r');

	writeb((u8)c, &uart->rbr_thr_dllr);

	/* Wait for fifo to shift out some bytes */
	while (!((readb(&uart->lsr) & (UART_LSR_TDRQ | UART_LSR_TEMT)) == 0x60))
		;
}

static int jz_serial_getc(void)
{
	while (!serial_tstc())
		;

	return readb(&uart->rbr_thr_dllr);
}

static struct serial_device jz_serial_drv = {
	.name	= "jz_serial",
	.start	= jz_serial_init,
	.stop	= NULL,
	.setbrg	= jz_serial_setbrg,
	.putc	= jz_serial_putc,
	.puts	= default_serial_puts,
	.getc	= jz_serial_getc,
	.tstc	= jz_serial_tstc,
};

void jz_serial_initialize(void)
{
	serial_register(&jz_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &jz_serial_drv;
}
