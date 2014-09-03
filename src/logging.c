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

void start_logging(struct gengetopt_args_info args_info) {
        GLogLevelFlags level = G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION;
        if (args_info.verbose_given) level = G_LOG_LEVEL_INFO    | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION;
        if (args_info.debug_given)   level = G_LOG_LEVEL_DEBUG   | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION;
        if (args_info.quiet_given)   level = G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION;
        g_log_set_handler(G_LOG_DOMAIN, level, g_log_default_handler, NULL);
}

#ifdef TEST_EVERYTHING
int main(void) {
        return 0;
}
#endif
