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

#include "getopt/cmdline.h"
#include "cppp.h"
#include "logging.h"
#include "decision_tree.h"

static gint cmp(gconstpointer a, gconstpointer b) {
        return (GPOINTER_TO_INT(a) < GPOINTER_TO_INT(b)) ? -1 : (GPOINTER_TO_INT(a) > GPOINTER_TO_INT(b));
}

static GSList*
alphabetic(state_s *stp) {
        return (g_slist_sort(stp->instance->characters, cmp));
}


int main(int argc, char **argv) {
        static struct gengetopt_args_info args_info;
        assert(cmdline_parser(argc, argv, &args_info) == 0);
        assert(args_info.inputs_num >= 1);
        start_logging(args_info);

        igraph_i_set_attribute_table(&igraph_cattribute_table);
        pp_instance temp = read_instance_from_filename(args_info.inputs[0]);

/**
   Notice that each character is realized at most twice (once positive and once
   negative) and that each species can be declared null at most once.

   Therefore each partial solution con contain at most 2n+m statuses.
*/
        state_s states[2*temp.num_species+temp.num_characters];

        exhaustive_search(states, temp, alphabetic);
        cmdline_parser_free(&args_info);
        return 0;
}
