/*
  cppp - Compute a Constrained Perfect Phylogeny, if it exists

  Copyright (C) 2014 Gianluca Della Vedova

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/
#include "logging.h"

#define LOG_ALL       0
#define LOG_DEBUG     1
#define LOG_INFO      2
#define LOG_WARN      3
#define LOG_ERROR     4
#define LOG_FATAL     5
#define LOG_NEXTFREE  6

static int _cppp_log_level_ = LOG_ERROR;

/**
   All logging functions return 1 if something is output to the log
   file, and 0 otherwise.
*/
static unsigned int log_format(const char* tag, int level, const char* message, va_list args) {
        if (level >= _cppp_log_level_) {
                fprintf(stderr, "[%s] ", tag);
                vfprintf(stderr, message, args);
                fprintf(stderr, "\n");
                return 1;
        }
        return 0;
}

unsigned int log_error(const char* message, ...) {
        va_list args; va_start(args, message);
        unsigned int ret = log_format("error", LOG_ERROR, message, args);
        va_end(args);
        return ret;
}
unsigned int log_info(const char* message, ...) {
        va_list args; va_start(args, message);
        unsigned int ret = log_format("info", LOG_INFO, message, args);
        va_end(args);
        return ret;
}
unsigned int log_debug(const char* message, ...) {
        va_list args; va_start(args, message);
        unsigned int ret = log_format("debug", LOG_DEBUG, message, args);
        va_end(args);
        return ret;
}

void start_logging(struct gengetopt_args_info args_info) {
        if (args_info.quiet_given)   _cppp_log_level_ = LOG_FATAL;
        if (args_info.verbose_given) _cppp_log_level_ = LOG_INFO;
        if (args_info.debug_given)   _cppp_log_level_ = LOG_DEBUG;
        if (getenv("CPPP_LOG_LEVEL") != NULL)
                _cppp_log_level_ = atoi(getenv("CPPP_LOG_LEVEL"));
}

void log_array(const char* name, const uint32_t* arr, const uint32_t size) {
        fprintf(stderr, "  %s. Size %d  Address %p Values: ", name, size, arr);
        if (arr != NULL)
                for(uint32_t i = 0; i < size; i++)
                        fprintf(stderr, "%d ", arr[i]);
        else
                fprintf(stderr, "NULL");
        fprintf(stderr, "\n");
}
