#ifndef _IPXE_SERIAL_H
#define _IPXE_SERIAL_H

/** @file
 *
 * Serial driver functions
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>

#define	COM1		0x3f8
#define	COM2		0x2f8
#define	COM3		0x3e8
#define	COM4		0x2e8

struct serial_config {
	u32 speed;
	u16 console;
	u8 stop;
	u8 data;
	enum {
		PARITY_NONE,
		PARITY_ODD,
		PARITY_EVEN,
	} parity;
	unsigned preserve:1;
};

extern struct serial_config default_serial;

extern void serial_putc ( int ch );
extern int serial_getc ( void );
extern int serial_ischar ( void );

extern void serial_putc_ ( struct serial_config *port, int ch );
extern int serial_getc_ ( struct serial_config *port );
extern int serial_ischar_ ( struct serial_config *port );
extern void serial_init_ ( struct serial_config *port );

#endif /* _IPXE_SERIAL_H */
