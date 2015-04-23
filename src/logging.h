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
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "getopt/cmdline.h"
#include <stdarg.h>

#define LOG_ALL       0
#define LOG_DEBUG     1
#define LOG_INFO      2
#define LOG_WARN      3
#define LOG_ERROR     4
#define LOG_FATAL     5
#define LOG_NEXTFREE  6

static int _cppp_log_level_ = 4;

void log_error(const char* message, ...);
void log_info(const char* message, ...);
void log_debug(const char* message, ...);
void start_logging(struct gengetopt_args_info args_info);
