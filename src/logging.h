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

#ifndef _JACKOFF_LOGGING_H_
#define _JACKOFF_LOGGING_H_

typedef enum {
	JACKOFF_LOG_DEBUG = 1,
	JACKOFF_LOG_INFO,
	JACKOFF_LOG_WARNING,
	JACKOFF_LOG_ERROR
} jackoff_log_level;

#define jackoff_debug(...) jackoff_log(JACKOFF_LOG_DEBUG, __VA_ARGS__)
#define jackoff_info(...) jackoff_log(JACKOFF_LOG_INFO, __VA_ARGS__)
#define jackoff_warn(...) jackoff_log(JACKOFF_LOG_WARNING, __VA_ARGS__)
#define jackoff_error(...) jackoff_log(JACKOFF_LOG_ERROR, __VA_ARGS__)

void jackoff_log(jackoff_log_level level, const char* format, ...);
void jackoff_set_log_cutoff(jackoff_log_level minimum_level);

#endif
