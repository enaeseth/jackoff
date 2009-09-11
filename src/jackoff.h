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

#ifndef _JACKOFF_H_
#define _JACKOFF_H_

//#include "config.h"
#include "client.h"

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#define JACKOFF_DEFAULT_CLIENT_NAME "jackoff"
#define JACKOFF_DEFAULT_FORMAT "aiff"
#define JACKOFF_DEFAULT_BITRATE_PER_CHANNEL 128
#define JACKOFF_DEFAULT_CHANNELS 2
#define JACKOFF_DEFAULT_RING_BUFFER_DURATION 2.0
#define JACKOFF_WRITE_BUFFER_SIZE 4098

typedef struct jackoff_output_format jackoff_format_t;
typedef struct jackoff_session jackoff_session_t;
typedef struct jackoff_encoder jackoff_encoder_t;

struct jackoff_output_format {
	const char* name;
	const char* description;
	jackoff_encoder_t* (*create_encoder)(jackoff_client_t* client,
		jackoff_format_t* format, int bitrate);
	int options;
};


struct jackoff_encoder {
	jackoff_session_t* (*open)(jackoff_client_t* client,
		jackoff_encoder_t* encoder, const char* file_path);
	int (*close)(const jackoff_session_t* session);
	long (*write)(const jackoff_session_t* session);
	void (*shutdown)(const jackoff_encoder_t* encoder);
};

struct jackoff_session {
	jackoff_client_t* client;
	jackoff_encoder_t* encoder;
	const char* file_path;
};

jackoff_format_t* jackoff_get_output_format(const char* name);

jackoff_encoder_t* jackoff_create_encoder(jackoff_client_t* client,
	jackoff_format_t* format, int bitrate);
void jackoff_destroy_encoder(jackoff_encoder_t* encoder);

jackoff_session_t* jackoff_open_session(jackoff_client_t* client,
	jackoff_encoder_t* encoder, const char* file_path);
int jackoff_close_session(jackoff_session_t* session);
long jackoff_write_session(jackoff_session_t* session);

void jackoff_shutdown();

#endif
