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

#include "jackoff.h"
#include "sndfile.h"
#include "logging.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sndfile.h>

// Write 256 samples to disk at a time
static const jack_nframes_t read_size = 256;

typedef struct sndfile_encoder {
	struct jackoff_encoder encoder;
	SF_INFO info;
} sndfile_encoder_t;

typedef struct sndfile_session {
	struct jackoff_session session;
	SNDFILE* sndfile;
	jack_default_audio_sample_t* interleaved_buffer;
	jack_default_audio_sample_t** channel_buffers;
} sndfile_session_t;

static jackoff_session_t* jackoff_sndfile_open(jackoff_client_t* client,
	jackoff_encoder_t* encoder, const char* file_path);
static int jackoff_sndfile_close(const jackoff_session_t* session);
static long jackoff_sndfile_write(const jackoff_session_t* session);
static void jackoff_sndfile_shutdown(const jackoff_encoder_t* encoder);

jackoff_encoder_t* jackoff_create_sndfile_encoder(jackoff_client_t* client,
	jackoff_format_t* format, int bitrate)
{
	sndfile_encoder_t* encoder;
	char sndfile_version[128];
	
	encoder = calloc(1, sizeof(sndfile_encoder_t));
	if (!encoder) {
		jackoff_error("Failed to allocate memory for libsndfile encoder.");
        return NULL;
	}
	
	encoder->info.format = format->options;
	
	sf_command(NULL, SFC_GET_LIB_VERSION, sndfile_version,
		sizeof(sndfile_version));
	jackoff_debug("Created a new encoder with %s.", sndfile_version);
	
	encoder->info.samplerate = jack_get_sample_rate(client->jack_client);
	encoder->info.channels = (int) client->channel_count;
	
	encoder->encoder.open = jackoff_sndfile_open;
	encoder->encoder.close = jackoff_sndfile_close;
	encoder->encoder.write = jackoff_sndfile_write;
	encoder->encoder.shutdown = jackoff_sndfile_shutdown;
	
	return (jackoff_encoder_t*) encoder;
}

static jackoff_session_t* jackoff_sndfile_open(jackoff_client_t* client,
	jackoff_encoder_t* base_encoder, const char* file_path)
{
    sndfile_encoder_t* encoder = (sndfile_encoder_t*) base_encoder;
	sndfile_session_t* session;
	
	session = calloc(1, sizeof(sndfile_session_t));
	if (!session) {
		jackoff_error("Failed to allocate a libsndfile-based session.");
        return NULL;
	}
	
	session->channel_buffers = calloc(client->channel_count,
		sizeof(jack_default_audio_sample_t*));
	if (!session->channel_buffers) {
		free(session);
		jackoff_error("Failed to allocate channel buffers.");
	}
	
	session->sndfile = sf_open(file_path, SFM_WRITE, &encoder->info);
	if (!session->sndfile) {
		jackoff_warn("Failed to open output file: %s", sf_strerror(NULL));
		free(session->channel_buffers);
		free(session);
		return NULL;
	}
	
    jackoff_debug("Created a new libsndfile session recording to \"%s\".",
		file_path);
	return (jackoff_session_t*) session;
}

static int jackoff_sndfile_close(const jackoff_session_t* base_session) {
	sndfile_session_t* session = (sndfile_session_t*) base_session;
	size_t i;
	
	sf_write_sync(session->sndfile);
	if (sf_close(session->sndfile) != 0) {
		jackoff_warn("Failed to close output file: %s",
			sf_strerror(session->sndfile));
		return -1;
	}
	
	if (session->channel_buffers) {
		for (i = 0; i < base_session->client->channel_count; i++) {
			if (session->channel_buffers[i])
				free(session->channel_buffers[i]);
		}
		free(session->channel_buffers);
	}
	
	if (session->interleaved_buffer)
		free(session->interleaved_buffer);
	
	return 0;
}

static long jackoff_sndfile_write(const jackoff_session_t* base_session) {
	sndfile_session_t* session = (sndfile_session_t*) base_session;
	
	size_t desired = read_size * sizeof(jack_default_audio_sample_t);
	sf_count_t frames_written = 0;
	size_t i, c, bytes_read = 0;
	jackoff_client_t* client = base_session->client;
	size_t channels = client->channel_count;
	
	// Check to make sure there's enough space in the ring buffers.
	for (c = 0; c < channels; c++) {
		if (jack_ringbuffer_read_space(client->ring_buffers[c]) < desired) {
			// Try again later.
			return 0;
		}
	}
	
	for (c = 0; c < channels; c++) {
		session->channel_buffers[c] = (jack_default_audio_sample_t*)
			realloc(session->channel_buffers[c], desired);
		if (!session->channel_buffers[c]) {
			jackoff_error("Failed to reallocate channel buffer %lu.", c);
		}
		
		bytes_read = jack_ringbuffer_read(client->ring_buffers[c],
			(char*) session->channel_buffers[c], desired);
		if (bytes_read != desired) {
			jackoff_error("Failed to read desired # of bytes from RB %lu.",
				c);
		}
	}
	
	session->interleaved_buffer = (jack_default_audio_sample_t*)
		realloc(session->interleaved_buffer, desired * channels);
	if (!session->interleaved_buffer)
		jackoff_error("Failed to reallocate interleaved buffer.");
	for (c = 0; c < channels; c++) {
		for (i = 0; i < read_size; i++) {
			session->interleaved_buffer[(i * channels) + c] =
				session->channel_buffers[c][i];
		}
	}
	
	frames_written = sf_writef_float(session->sndfile,
        session->interleaved_buffer, read_size);
	if (frames_written != read_size) {
		jackoff_warn("Failed to write audio to disk: %s",
			sf_strerror(session->sndfile));
		return -1;
	}
	
	return frames_written;
}

static void jackoff_sndfile_shutdown(const jackoff_encoder_t* encoder) {
	// We don't actually need to do anything here.
}
