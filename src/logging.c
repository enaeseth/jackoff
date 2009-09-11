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
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

static jackoff_log_level minimum_logging_level = JACKOFF_LOG_INFO;
static FILE* log_output_stream = (FILE*) 0;

void jackoff_set_log_cutoff(jackoff_log_level minimum_level) {
	minimum_logging_level = minimum_level;
}

void jackoff_log(jackoff_log_level level, const char* format, ...) {
	time_t now = time(NULL);
	char time_string[64];
	va_list args;
	
	if (!log_output_stream) {
		log_output_stream = stderr;
	}
	
	switch (level) {
		case JACKOFF_LOG_DEBUG:
			fprintf(log_output_stream, "[DEBUG] ");
			break;
		case JACKOFF_LOG_INFO:
			fprintf(log_output_stream, "[INFO]  ");
			break;
		case JACKOFF_LOG_WARNING:
			fprintf(log_output_stream, "[WARN]  ");
			break;
		case JACKOFF_LOG_ERROR:
			fprintf(log_output_stream, "[ERROR] ");
			break;
		default:
			// wtf
			fprintf(log_output_stream, "        ");
	}
	
	if (ctime_r(&now, time_string)) {
		time_string[strlen(time_string) - 1] = 0; // remove newline at end
		fprintf(log_output_stream, "%s  ", time_string);
	}
	
	va_start(args, format);
	vfprintf(log_output_stream, format, args);
	fprintf(log_output_stream, "\n");
	va_end(args);
	
	if (level >= JACKOFF_LOG_ERROR) {
		jackoff_shutdown();
	}
}
