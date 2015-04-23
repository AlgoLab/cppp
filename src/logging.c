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

static void log_format(const char* tag, int level, const char* message, va_list args) {
        if (level >= _cppp_log_level_) {
                time_t now;
                time(&now);
                char * date =ctime(&now);
                date[strlen(date) - 1] = '\0';
                printf("%s [%s] ", date, tag);
                vprintf(message, args);
                printf("\n");
        }
}

void log_error(const char* message, ...) {
        va_list args; va_start(args, message);
        log_format("error", LOG_ERROR, message, args);
        va_end(args);
}
void log_info(const char* message, ...) {
        va_list args; va_start(args, message);
        log_format("info", LOG_INFO, message, args);
        va_end(args);
}
void log_debug(const char* message, ...) {
        va_list args; va_start(args, message);
        log_format("debug", LOG_DEBUG, message, args);
        va_end(args);
}

void start_logging(struct gengetopt_args_info args_info) {
        if (args_info.quiet_given)   _cppp_log_level_ = 5;
        if (args_info.verbose_given) _cppp_log_level_ = 2;
        if (args_info.debug_given)   _cppp_log_level_ = 1;
        if (getenv("CPPP_LOG_LEVEL") != NULL)
                _cppp_log_level_ = atoi(getenv("CPPP_LOG_LEVEL"));
}


/*
  This file is mainly used as a library.

  The following main is used only for using the testing framework.
*/
#ifdef TEST_EVERYTHING
int main(int argc, char **argv) {
        return EXIT_SUCCESS;
}
#endif
