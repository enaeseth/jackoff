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

#include "client.h"
#include "logging.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int audio_available_callback(jack_nframes_t frame_count, void* arg);
static void jackd_shutdown_callback(void* arg);
static void client_open_failed(jack_status_t status);
static void get_input_port_name(size_t total, size_t index, char* buffer,
	size_t buffer_length);

jackoff_client_t* jackoff_create_client(const char* client_name,
	jack_options_t jack_options, size_t channels, float buffer_duration)
{
	jackoff_client_t* client;
	jack_client_t* jack_client;
	jack_status_t status;
	char input_port_name[64];
	size_t i;
	jack_port_t* port;
	jack_ringbuffer_t* buffer;
	size_t buffer_size;
	
	client = calloc(1, sizeof(jackoff_client_t));
	if (!client) {
		jackoff_error("Failed to allocate memory for Jackoff client.");
		return NULL;
	}
	
	jack_client = jack_client_open(client_name, jack_options, &status);
	if (!jack_client) {
		free(client);
		client_open_failed(status);
		return NULL;
	}
	
	jackoff_info("JACK client registered as \"%s\".",
		jack_get_client_name(jack_client));
	
	client->jack_client = jack_client;
	client->channel_count = channels;
	client->input_ports = calloc(channels, sizeof(jack_port_t*));
	client->ring_buffers = calloc(channels, sizeof(jack_ringbuffer_t*));
	
	buffer_size = jack_get_sample_rate(jack_client) * buffer_duration *
		sizeof(float);
	jackoff_debug("Ring buffer size: %2.2f seconds; %lu bytes.",
		buffer_duration, buffer_size);
	
	for (i = 0; i < channels; i++) {
		get_input_port_name(channels, i, input_port_name, 64);
		port = jack_port_register(jack_client, input_port_name,
			JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		if (!port) {
			jackoff_error("Failed to register JACK input port \"%s\".",
				input_port_name);
		}
		client->input_ports[i] = port;
		
		buffer = jack_ringbuffer_create(buffer_size);
		if (!buffer) {
			jackoff_error("Failed to create JACK ring buffer for channel %lu.",
				i);
		}
		client->ring_buffers[i] = buffer;
	}
	
	client->status = 1;
	
	jack_on_shutdown(jack_client, jackd_shutdown_callback, client);
	jack_set_process_callback(jack_client, audio_available_callback, client);
	
	return client;
}

static void client_open_failed(jack_status_t status)
{
	const char* prefix = "Failed to open JACK client";
	
	if (status & JackInvalidOption) {
		jackoff_error("%s: invalid option.", prefix);
	} else if (status & JackNameNotUnique) {
		jackoff_error("%s: client name is not unique.", prefix);
	} else if (status & JackServerFailed) {
		jackoff_error("%s: unable to connect to the JACK server.", prefix);
	} else if (status & JackServerError) {
		jackoff_error("%s: communication error with the JACK server.", prefix);
	} else if (status & JackInitFailure) {
		jackoff_error("%s: client initialization failed.", prefix);
	} else if (status & JackShmFailure) {
		jackoff_error("%s: unable to access shared memory.", prefix);
	} else if (status & JackVersionError) {
		jackoff_error("%s: protocol version mismatch.", prefix);
	} else {
		jackoff_error("%s: operation failed (0x%04X).", prefix);
	}
}

static void get_input_port_name(size_t total, size_t index, char* buffer,
	size_t buffer_length)
{
	if (total == 1) {
		strncpy(buffer, "mono", buffer_length);
	} else if (total == 2) {
		if (index == 0)
			strncpy(buffer, "left", buffer_length);
		else
			strncpy(buffer, "right", buffer_length);
	} else {
		snprintf(buffer, buffer_length, "channel_%lu", (index + 1));
	}
}

int jackoff_activate_client(jackoff_client_t* client) {
	return jack_activate(client->jack_client);
}

void jackoff_destroy_client(jackoff_client_t* client) {
	size_t i;
	size_t channels;
	
	if (!client) {
		jackoff_warn("Tried to destroy a NULL client.");
		return;
	}
	
	channels = client->channel_count;
	
	for (i = 0; i < channels; i++) {
		jack_port_unregister(client->jack_client, client->input_ports[i]);
	}
	free(client->input_ports);
	
	jack_client_close(client->jack_client);
	
	for (i = 0; i < channels; i++) {
		jack_ringbuffer_free(client->ring_buffers[i]);
	}
	free(client->ring_buffers);
	
	free(client);
}

void jackoff_auto_connect_client_ports(jackoff_client_t* client) {
	const char** port_names;
	size_t i;
	
	port_names = jack_get_ports(client->jack_client, NULL, NULL,
		JackPortIsOutput);
	if (!port_names) {
		jackoff_error("Failed to get the list of JACK output ports.");
		return;
	}
	
	for (i = 0; port_names[i] != NULL && i < client->channel_count; i++) {
		jackoff_connect_client_port(client, i, port_names[i]);
	}
	
	free(port_names);
}

void jackoff_connect_client_port(jackoff_client_t* client, size_t channel,
	const char* output_port_name)
{
	const char* input_port_name = jack_port_name(client->input_ports[channel]);
	int result;
	
	jackoff_info("Connecting JACK port \"%s\" to \"%s\".", output_port_name,
		input_port_name);
	
	result = jack_connect(client->jack_client, output_port_name,
		input_port_name);
	if (result != 0) {
		jackoff_error("Failed to connect with port \"%s\": %d.",
			output_port_name, result);
	}
}

static int audio_available_callback(jack_nframes_t frame_count, void* arg) {
	jackoff_client_t* client = arg;
	
	size_t write_size = sizeof(jack_default_audio_sample_t) * frame_count;
	size_t channels = client->channel_count;
	size_t c;
	char* buffer;
	size_t space;
	size_t written;
	
	for (c = 0; c < channels; c++) {
		space = jack_ringbuffer_write_space(client->ring_buffers[c]);
		if (space < write_size) {
			client->ring_buffer_overflowed = 1;
			return 0;
		}
	}
	
	for (c = 0; c < channels; c++) {
		buffer = (char*) jack_port_get_buffer(client->input_ports[c],
			frame_count);
		written = jack_ringbuffer_write(client->ring_buffers[c], buffer,
			write_size);
		if (written < write_size) {
			jackoff_warn("Failed to write to a ring buffer.");
			return 1;
		}
	}
	
	return 0; // success
}

static void jackd_shutdown_callback(void* arg) {
	jackoff_client_t* client = arg;
	
	if (client->status) {
		jackoff_warn("jackd is shutting down; jackoff will now quit.");
		client->status = 0;
	}
}
