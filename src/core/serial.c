/*
 * The serial port interface routines implement a simple polled i/o
 * interface to a standard serial port.  Due to the space restrictions
 * for the boot blocks, no BIOS support is used (since BIOS requires
 * expensive real/protected mode switches), instead the rudimentary
 * BIOS support is duplicated here.
 *
 * The base address and speed for the i/o port are passed from the
 * Makefile in the COMCONSOLE and CONSPEED preprocessor macros.  The
 * line control parameters are currently hard-coded to 8 bits, no
 * parity, 1 stop bit (8N1).  This can be changed in init_serial().
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include "stddef.h"
#include <ipxe/init.h>
#include <ipxe/io.h>
#include <unistd.h>
#include <stdio.h>
#include <ipxe/serial.h>
#include "config/serial.h"

/* Set default values if none specified */

#ifndef COMCONSOLE
#define COMCONSOLE	0x3f8
#endif

#ifndef COMSPEED
#define COMSPEED	9600
#endif

#ifndef COMDATA
#define COMDATA		8
#endif

#ifndef COMPARITY
#define COMPARITY	0
#endif

#ifndef COMSTOP
#define COMSTOP		1
#endif

#if ((115200%COMSPEED) != 0)
#error Bad ttys0 baud rate
#endif

inline u8 uart_lcs ( struct serial_config *config ) {
	return ( ( config->data - 5 )	<< 0 )
	     | ( ( config->parity )	<< 3 )
	     | ( ( config->stop - 1 )	<< 2 );
}

struct serial_config default_serial = {
	.console = COMCONSOLE,
	.speed = COMSPEED,
	.data = COMDATA,
	.parity = COMPARITY,
	.stop = COMSTOP,
#ifdef COMPRESERVE
	.preserve = 1,
#endif
};

/* Data */
#define UART_RBR 0x00
#define UART_TBR 0x00

/* Control */
#define UART_IER 0x01
#define UART_IIR 0x02
#define UART_FCR 0x02
#define UART_LCR 0x03
#define UART_MCR 0x04
#define UART_DLL 0x00
#define UART_DLM 0x01

/* Status */
#define UART_LSR 0x05
#define  UART_LSR_TEMPT 0x40	/* Transmitter empty */
#define  UART_LSR_THRE  0x20	/* Transmit-hold-register empty */
#define  UART_LSR_BI	0x10	/* Break interrupt indicator */
#define  UART_LSR_FE	0x08	/* Frame error indicator */
#define  UART_LSR_PE	0x04	/* Parity error indicator */
#define  UART_LSR_OE	0x02	/* Overrun error indicator */
#define  UART_LSR_DR	0x01	/* Receiver data ready */

#define UART_MSR 0x06
#define UART_SCR 0x07

#if defined(UART_MEM)
#define uart_readb(addr) readb((addr))
#define uart_writeb(val,addr) writeb((val),(addr))
#else
#define uart_readb(addr) inb((addr))
#define uart_writeb(val,addr) outb((val),(addr))
#endif

void serial_putc ( int ch ) { serial_putc_(&default_serial, ch); }
int serial_getc ( void ) { return serial_getc_(&default_serial); }
int serial_ischar ( void ) { return serial_ischar_(&default_serial); }
static void serial_init ( void ) { return serial_init_(&default_serial); }

/*
 * void serial_putc(int ch);
 *	Write character `ch' to port port->console.
 */
void serial_putc_ ( struct serial_config *port, int ch ) {
	int i;
	int status;
	i = 1000; /* timeout */
	while(--i > 0) {
		status = uart_readb(port->console + UART_LSR);
		if (status & UART_LSR_THRE) {
			/* TX buffer emtpy */
			uart_writeb(ch, port->console + UART_TBR);
			break;
		}
		mdelay(2);
	}
}

/*
 * int serial_getc(void);
 *	Read a character from port port->console.
 */
int serial_getc_ ( struct serial_config *port ) {
	int status;
	int ch;
	do {
		status = uart_readb(port->console + UART_LSR);
	} while((status & 1) == 0);
	ch = uart_readb(port->console + UART_RBR); /* fetch (first) character */
	ch &= 0x7f;				   /* remove any parity bits we get */
	if (ch == 0x7f) {			   /* Make DEL... look like BS */
		ch = 0x08;
	}
	return ch;
}

/*
 * int serial_ischar(void);
 *       If there is a character in the input buffer of port port->console,
 *       return nonzero; otherwise return 0.
 */
int serial_ischar_ ( struct serial_config *port ) {
	int status;
	status = uart_readb(port->console + UART_LSR); /* line status reg; */
	return status & 1;			       /* rx char available */
}

/*
 * int serial_init(void);
 *	Initialize port port->console to speed port->speed, line settings 8N1.
 */
void serial_init_ ( struct serial_config *port ) {
	int status;
	int divisor, lcs;

	DBG ( "Serial port %#x initialising\n", port->console );

	if ((115200 % port->speed) != 0) {
		printf("Bad baud rate %d\n", port->speed);
		return;
	}

	divisor = 115200 / port->speed;
	lcs = uart_lcs(port);

	if (port->preserve) {
		lcs = uart_readb(port->console + UART_LCR) & 0x7f;
		uart_writeb(0x80 | lcs, port->console + UART_LCR);
		divisor = (uart_readb(port->console + UART_DLM) << 8)
			| uart_readb(port->console + UART_DLL);
		uart_writeb(lcs, port->console + UART_LCR);
	}

	/* Set Baud Rate Divisor to port->speed, and test to see if the
	 * serial port appears to be present.
	 */
	uart_writeb(0x80 | lcs, port->console + UART_LCR);
	uart_writeb(0xaa, port->console + UART_DLL);
	if (uart_readb(port->console + UART_DLL) != 0xaa) {
		DBG ( "Serial port %#x UART_DLL failed\n", port->console );
		goto out;
	}
	uart_writeb(0x55, port->console + UART_DLL);
	if (uart_readb(port->console + UART_DLL) != 0x55) {
		DBG ( "Serial port %#x UART_DLL failed\n", port->console );
		goto out;
	}
	uart_writeb(divisor & 0xff, port->console + UART_DLL);
	if (uart_readb(port->console + UART_DLL) != (divisor & 0xff)) {
		DBG ( "Serial port %#x UART_DLL failed\n", port->console );
		goto out;
	}
	uart_writeb(0xaa, port->console + UART_DLM);
	if (uart_readb(port->console + UART_DLM) != 0xaa) {
		DBG ( "Serial port %#x UART_DLM failed\n", port->console );
		goto out;
	}
	uart_writeb(0x55, port->console + UART_DLM);
	if (uart_readb(port->console + UART_DLM) != 0x55) {
		DBG ( "Serial port %#x UART_DLM failed\n", port->console );
		goto out;
	}
	uart_writeb((divisor >> 8) & 0xff, port->console + UART_DLM);
	if (uart_readb(port->console + UART_DLM) != ((divisor >> 8) & 0xff)) {
		DBG ( "Serial port %#x UART_DLM failed\n", port->console );
		goto out;
	}
	uart_writeb(lcs, port->console + UART_LCR);

	/* disable interrupts */
	uart_writeb(0x0, port->console + UART_IER);

	/* disable fifo's */
	uart_writeb(0x00, port->console + UART_FCR);

	/* Set clear to send, so flow control works... */
	uart_writeb((1<<1), port->console + UART_MCR);


	/* Flush the input buffer. */
	do {
		/* rx buffer reg
		 * throw away (unconditionally the first time)
		 */
	        (void) uart_readb(port->console + UART_RBR);
		/* line status reg */
		status = uart_readb(port->console + UART_LSR);
	} while(status & UART_LSR_DR);
 out:
	return;
}

/*
 * void serial_fini(void);
 *	Cleanup our use of the serial port, in particular flush the
 *	output buffer so we don't accidentially lose characters.
 */
static void serial_fini ( int flags __unused ) {
	int i, status;
	/* Flush the output buffer to avoid dropping characters,
	 * if we are reinitializing the serial port.
	 */
	i = 10000; /* timeout */
	do {
		status = uart_readb(default_serial.console + UART_LSR);
	} while((--i > 0) && !(status & UART_LSR_TEMPT));
	/* Don't mark it as disabled; it's still usable */
}

/**
 * Serial driver initialisation function
 *
 * Initialise serial port early on so that it is available to capture
 * early debug messages.
 */
struct init_fn serial_init_fn __init_fn ( INIT_SERIAL ) = {
	.initialise = serial_init,
};

/** Serial driver startup function */
struct startup_fn serial_startup_fn __startup_fn ( STARTUP_EARLY ) = {
	.shutdown = serial_fini,
};
