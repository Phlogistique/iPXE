/*
 * Copyright (C) 2008 Stefan Hajnoczi <stefanha@gmail.com>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ipxe/serial.h>
#include <ipxe/gdbstub.h>
#include <ipxe/gdbserial.h>

struct gdb_transport serial_gdb_transport __gdb_transport;

static struct serial_config port;

static size_t gdbserial_recv ( char *buf, size_t len ) {
	assert ( len > 0 );
	buf [ 0 ] = serial_getc_( &port );
	return 1;
}

static void gdbserial_send ( const char *buf, size_t len ) {
	while ( len-- > 0 ) {
		serial_putc_ ( &port, *buf++ );
	}
}

static int gdbserial_init ( int argc, char **argv ) {

	port = default_serial;

	switch ( argc ) {
	case 1:
		if (!strcmp("COM1", argv[0])) port.console = COM1;
		else if (!strcmp("COM2", argv[0])) port.console = COM2;
		else if (!strcmp("COM3", argv[0])) port.console = COM3;
		else if (!strcmp("COM4", argv[0])) port.console = COM4;
		else {
			printf("%s is not a serial port. Use COM1, COM2, COM3, or COM4\n", argv[0]);
			return 1;
		}

		port.preserve = 0; /* TODO: this should be configurable. */
		serial_init_(&port);
	case 0:
		break;
	default:
		printf("Usage: gdbstub serial [port]\n");
		return 1;
	}

	return 0;
}

struct gdb_transport serial_gdb_transport __gdb_transport = {
	.name = "serial",
	.init = gdbserial_init,
	.recv = gdbserial_recv,
	.send = gdbserial_send,
};

struct gdb_transport *gdbserial_configure ( void ) {
	return &serial_gdb_transport;
}
