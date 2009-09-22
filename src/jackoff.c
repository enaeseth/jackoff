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
#include "logging.h"
#include "driver_sndfile.h"

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sndfile.h>

jackoff_format_t available_formats[] = {
#ifdef HAVE_SNDFILE
	{"aiff", "AIFF (16-bit PCM)", jackoff_create_sndfile_encoder,
		SF_FORMAT_AIFF | SF_FORMAT_PCM_16},
	{"aiff32", "AIFF (32-bit float)", jackoff_create_sndfile_encoder,
		SF_FORMAT_AIFF | SF_FORMAT_FLOAT},
	{"au", "AU (16-bit PCM)", jackoff_create_sndfile_encoder,
		SF_FORMAT_AU | SF_FORMAT_PCM_16},
	{"au24", "AU (24-bit PCM)", jackoff_create_sndfile_encoder,
		SF_FORMAT_AU | SF_FORMAT_PCM_24},
	{"au32", "AU (32-bit float)", jackoff_create_sndfile_encoder,
		SF_FORMAT_AU | SF_FORMAT_FLOAT},
	{"caf", "Core Audio (16-bit PCM)", jackoff_create_sndfile_encoder,
		SF_FORMAT_AU | SF_FORMAT_PCM_16},
	{"caf32", "Core Audio (32-bit float)", jackoff_create_sndfile_encoder,
		SF_FORMAT_AU | SF_FORMAT_FLOAT},
#ifdef SF_FORMAT_FLAC
	{"flac", "FLAC (16-bit PCM)", jackoff_create_sndfile_encoder,
		SF_FORMAT_FLAC | SF_FORMAT_PCM_16},
#endif
#ifdef SF_FORMAT_VORBIS
	{"vorbis", "Ogg Vorbis", jackoff_create_sndfile_encoder,
		SF_FORMAT_OGG | SF_FORMAT_VORBIS},
#endif
	{"wav", "WAV (16-bit PCM)", jackoff_create_sndfile_encoder,
		SF_FORMAT_WAV | SF_FORMAT_PCM_16},
	{"wav32", "WAV (32-bit float)", jackoff_create_sndfile_encoder,
		SF_FORMAT_WAV | SF_FORMAT_FLOAT},
#endif
	{NULL, NULL, NULL, 0}
};

volatile int running = 0;

static void handle_signal(int signum);
static void show_usage_info();

jackoff_format_t* jackoff_get_output_format(const char* name) {
	jackoff_format_t* format;
	for (format = &available_formats[0]; format->name; format++) {
		if (0 == strcmp(format->name, name))
			return format;
	}
	return NULL;
}

jackoff_encoder_t* jackoff_create_encoder(jackoff_client_t* client,
	jackoff_format_t* format, int bitrate)
{
	jackoff_encoder_t* encoder = format->create_encoder(client, format,
		bitrate);
	return encoder;
}

void jackoff_destroy_encoder(jackoff_encoder_t* encoder)
{
	encoder->shutdown(encoder);
	free(encoder);
}

jackoff_session_t* jackoff_open_session(jackoff_client_t* client,
	jackoff_encoder_t* encoder, const char* file_path)
{
	jackoff_session_t* session = encoder->open(client, encoder, file_path);
	
	session->client = client;
	session->encoder = encoder;
	session->file_path = file_path;
	
	return session;
}

int jackoff_close_session(jackoff_session_t* session)
{
	int result = session->encoder->close(session);
	free(session);
	return result;
}

long jackoff_write_session(jackoff_session_t* session)
{
	return session->encoder->write(session);
}

void jackoff_shutdown()
{
	if (running) {
		// Graceful shutdown.
		running = 0;
	} else {
		// Fall on our asses.
		exit(9);
	}
}

int run(size_t port_count, const char** ports, const char* client_name,
	const char* file_path, jackoff_format_t* format, int bitrate,
	size_t channels, float buffer_duration, time_t recording_duration,
	jack_options_t options)
{
	jackoff_client_t* client;
	jackoff_encoder_t* encoder;
	jackoff_session_t* session;
	time_t stop_time = (time_t) 0;
	size_t i;
	long result;
	
	if (recording_duration) {
		stop_time = time(NULL) + recording_duration;
	}
	
	client = jackoff_create_client(client_name, options, channels, buffer_duration);
	
	if (jackoff_activate_client(client) != 0) {
		jackoff_destroy_client(client);
		jackoff_error("Failed to activate JACK client.");
	}
	
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	
	if (port_count == 0) {
		jackoff_auto_connect_client_ports(client);
	} else {
		for (i = 0; i < port_count; i++) {
			jackoff_connect_client_port(client, i, ports[i]);
		}
	}
	
	encoder = jackoff_create_encoder(client, format, bitrate);
	if (!encoder) {
		jackoff_destroy_client(client);
		return 1;
	}
	
	session = jackoff_open_session(client, encoder, file_path);
	if (!session) {
		jackoff_destroy_encoder(encoder);
		jackoff_destroy_client(client);
		return 2;
	}
	
	running = 1;
	jackoff_info("Recording.");
	while (running && (stop_time == 0 || time(NULL) < stop_time)) {
		if (client->ring_buffer_overflowed) {
			jackoff_warn("Ring buffer overflow; some audio was not written.");
			client->ring_buffer_overflowed = 0;
		}
		
		result = jackoff_write_session(session);
		if (result == 0) {
			// Sleep for 1/4th the ring buffer duration.
			jackoff_debug("Sleeping for %.04fms.", (buffer_duration / 4));
			usleep(1000000 * (buffer_duration / 4));
		} else if (result == -1) {
			jackoff_close_session(session);
			jackoff_destroy_encoder(encoder);
			jackoff_destroy_client(client);
			jackoff_error("Encoding error. Shutting down.");
			return 3;
		}
	}
	
	jackoff_close_session(session);
	jackoff_destroy_encoder(encoder);
	jackoff_destroy_client(client);
	
	return 0;
}

static void handle_signal(int signum) {
	switch (signum) {
		case SIGHUP:
			jackoff_info("Got hangup signal; quitting.");
			break;
		case SIGTERM:
			jackoff_info("Got termination signal; quitting.");
			break;
		case SIGINT:
			jackoff_info("Got interrupt signal; quitting.");
			break;
		default:
			return;
	}
	
	signal(signum, handle_signal);
	jackoff_shutdown();
}

static const char* short_options = "an:f:b:c:d:R:p:Svqh";
static const struct option long_options[] = {
	{"auto-connect", no_argument, NULL, 'a'},
	{"client-name", required_argument, NULL, 'n'},
	{"format", required_argument, NULL, 'f'},
	{"bitrate", required_argument, NULL, 'b'},
	{"channels", required_argument, NULL, 'c'},
	{"duration", required_argument, NULL, 'd'},
	{"buffer-duration", required_argument, NULL, 'R'},
	{"ports", required_argument, NULL, 'p'},
	{"no-start-server", no_argument, NULL, 'S'},
	{"verbose", no_argument, NULL, 'v'},
	{"quiet", no_argument, NULL, 'q'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, 0}
};

int main(int argc, char* argv[]) {
	int auto_connect = 0;
	char* client_name = JACKOFF_DEFAULT_CLIENT_NAME;
	char* format_name = JACKOFF_DEFAULT_FORMAT;
	char* filename = NULL;
	int bitrate = -1;
	size_t channels = JACKOFF_DEFAULT_CHANNELS;
	float buffer_duration = JACKOFF_DEFAULT_RING_BUFFER_DURATION;
	jackoff_format_t* output_format;
	jack_options_t jack_options = JackNullOption;
	time_t duration = 0;
	
	int option, long_index;
	while (1) {
		option = getopt_long(argc, argv, short_options, long_options,
			&long_index);
		
		if (option == -1)
			break;
		
		switch (option) {
			case 'a':
				// auto_connect = 1;
				break;
			case 'n':
				client_name = optarg;
				break;
			case 'f':
				format_name = optarg;
				break;
			case 'b':
				bitrate = (int) strtol(optarg, NULL, 0);
				break;
			case 'c':
				channels = (size_t) strtol(optarg, NULL, 0);
				break;
			case 'd':
				duration = (time_t) strtol(optarg, NULL, 0);
				break;
			case 'R':
				buffer_duration = (float) strtod(optarg, NULL);
				break;
			case 'p':
				jackoff_error("manual port specification not yet implemented");
				break;
			case 'S':
				jack_options |= JackNoStartServer;
			case 'v':
				jackoff_set_log_cutoff(JACKOFF_LOG_DEBUG);
				break;
			case 'q':
				jackoff_set_log_cutoff(JACKOFF_LOG_WARNING);
				break;
			default:
				show_usage_info();
				return 10;
		}
	}
	
	if (bitrate == -1) {
		bitrate = JACKOFF_DEFAULT_BITRATE_PER_CHANNEL * (int) channels;
	}
	
	argc -= optind;
	argv += optind;
	if (argc != 1) {
		jackoff_error("must provide the name of a file to record to");
	} else {
		filename = argv[0];
	}
	
	output_format = jackoff_get_output_format(format_name);
	if (!output_format) {
		jackoff_error("unknown output format \"%s\"", format_name);
	}
	
	return run(0, NULL, client_name, filename, output_format, bitrate,
		channels, buffer_duration, duration, jack_options);
}

static void show_usage_info() {
	// to be implemented
}
