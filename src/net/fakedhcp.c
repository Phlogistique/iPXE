/*
 * Copyright (C) 2008 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <gpxe/settings.h>
#include <gpxe/netdevice.h>
#include <gpxe/dhcppkt.h>
#include <gpxe/fakedhcp.h>

/** @file
 *
 * Fake DHCP packets
 *
 */

/**
 * Copy settings to DHCP packet
 *
 * @v dest		Destination DHCP packet
 * @v source		Source settings block
 * @v encapsulator	Encapsulating setting tag number, or zero
 * @ret rc		Return status code
 */
static int copy_encap_settings ( struct dhcp_packet *dest,
				 struct settings *source,
				 unsigned int encapsulator ) {
	struct setting setting = { .name = "" };
	unsigned int subtag;
	unsigned int tag;
	int len;
	int check_len;
	int rc;

	for ( subtag = DHCP_MIN_OPTION; subtag <= DHCP_MAX_OPTION; subtag++ ) {
		tag = DHCP_ENCAP_OPT ( encapsulator, subtag );
		switch ( tag ) {
		case DHCP_EB_ENCAP:
		case DHCP_VENDOR_ENCAP:
			/* Process encapsulated settings */
			if ( ( rc = copy_encap_settings ( dest, source,
							  tag ) ) != 0 )
				return rc;
			break;
		default:
			/* Copy setting, if present */
			setting.tag = tag;
			len = fetch_setting_len ( source, &setting );
			if ( len < 0 )
				break;
			{
				char buf[len];

				check_len = fetch_setting ( source, &setting,
							    buf, sizeof (buf));
				assert ( check_len == len );
				if ( ( rc = dhcppkt_store ( dest, tag, buf,
							    sizeof(buf) )) !=0)
					return rc;
			}
			break;
		}
	}

	return 0;
}

/**
 * Copy settings to DHCP packet
 *
 * @v dest		Destination DHCP packet
 * @v source		Source settings block
 * @ret rc		Return status code
 */
static int copy_settings ( struct dhcp_packet *dest,
			   struct settings *source ) {
	return copy_encap_settings ( dest, source, 0 );
}

/**
 * Create fake DHCPDISCOVER packet
 *
 * @v netdev		Network device
 * @v data		Buffer for DHCP packet
 * @v max_len		Size of DHCP packet buffer
 * @ret rc		Return status code
 *
 * Used by external code.
 */
int create_fakedhcpdiscover ( struct net_device *netdev,
			      void *data, size_t max_len ) {
	struct dhcp_packet dhcppkt;
	int rc;

	if ( ( rc = create_dhcp_request ( &dhcppkt, netdev, NULL, data,
					  max_len ) ) != 0 ) {
		DBG ( "Could not create DHCPDISCOVER: %s\n",
		      strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Create fake DHCPACK packet
 *
 * @v netdev		Network device
 * @v data		Buffer for DHCP packet
 * @v max_len		Size of DHCP packet buffer
 * @ret rc		Return status code
 *
 * Used by external code.
 */
int create_fakedhcpack ( struct net_device *netdev,
			 void *data, size_t max_len ) {
	struct dhcp_packet dhcppkt;
	int rc;

	/* Create base DHCPACK packet */
	if ( ( rc = create_dhcp_packet ( &dhcppkt, netdev, DHCPACK, NULL,
					 data, max_len ) ) != 0 ) {
		DBG ( "Could not create DHCPACK: %s\n", strerror ( rc ) );
		return rc;
	}

	/* Merge in globally-scoped settings, then netdev-specific
	 * settings.  Do it in this order so that netdev-specific
	 * settings take precedence regardless of stated priorities.
	 */
	if ( ( rc = copy_settings ( &dhcppkt, NULL ) ) != 0 ) {
		DBG ( "Could not set DHCPACK global settings: %s\n",
		      strerror ( rc ) );
		return rc;
	}
	if ( ( rc = copy_settings ( &dhcppkt,
				    netdev_settings ( netdev ) ) ) != 0 ) {
		DBG ( "Could not set DHCPACK netdev settings: %s\n",
		      strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Create ProxyDHCPACK packet
 *
 * @v netdev		Network device
 * @v data		Buffer for DHCP packet
 * @v max_len		Size of DHCP packet buffer
 * @ret rc		Return status code
 *
 * Used by external code.
 */
int create_fakeproxydhcpack ( struct net_device *netdev,
			      void *data, size_t max_len ) {
	struct dhcp_packet dhcppkt;
	struct settings *settings;
	int rc;

	/* Identify ProxyDHCP settings */
	settings = find_settings ( PROXYDHCP_SETTINGS_NAME );

	/* No ProxyDHCP settings => return empty block */
	if ( ! settings ) {
		memset ( data, 0, max_len );
		return 0;
	}

	/* Create base DHCPACK packet */
	if ( ( rc = create_dhcp_packet ( &dhcppkt, netdev, DHCPACK, NULL,
					 data, max_len ) ) != 0 ) {
		DBG ( "Could not create ProxyDHCPACK: %s\n",
		      strerror ( rc ) );
		return rc;
	}

	/* Merge in ProxyDHCP options */
	if ( ( rc = copy_settings ( &dhcppkt, settings ) ) != 0 ) {
		DBG ( "Could not set ProxyDHCPACK settings: %s\n",
		      strerror ( rc ) );
		return rc;
	}

	return 0;
}