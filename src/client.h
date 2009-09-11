/*
 * Jackoff: a simple utility to record audio from JACK.
 * Copyright © 2009 Eric Naeseth.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _JACKOFF_CLIENT_H_
#define _JACKOFF_CLIENT_H_

#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <stdlib.h>

typedef struct {
	jack_client_t* jack_client;
	size_t channel_count;
	int status;
	jack_port_t** input_ports;
	jack_ringbuffer_t** ring_buffers;
	int ring_buffer_overflowed;
} jackoff_client_t;

jackoff_client_t* jackoff_create_client(const char* client_name,
	jack_options_t jack_options, size_t channels, float buffer_duration);
int jackoff_activate_client(jackoff_client_t* client);
void jackoff_destroy_client(jackoff_client_t* client);
void jackoff_auto_connect_client_ports(jackoff_client_t* client);
void jackoff_connect_client_port(jackoff_client_t* client, size_t channel,
	const char* output_port_name);

#endif
