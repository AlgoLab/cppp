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
#include "gc/leak_detector.h"
static GSList*
alphabetic(state_s *stp) {
        return (characters_list(stp));
}

int main(int argc, char **argv) {
        GC_find_leak = 1;
        igraph_i_set_attribute_table(&igraph_cattribute_table);
        static struct gengetopt_args_info args_info;
        assert(cmdline_parser(argc, argv, &args_info) == 0);
        assert(args_info.inputs_num >= 1);
        start_logging(args_info);
        FILE* outf = fopen(args_info.output_arg, "w");

        instances_schema_s props = {
                .file = NULL,
                .filename = args_info.inputs[0]
        };
        for (state_s* temp = read_instance_from_filename(&props);
             temp != NULL; temp = read_instance_from_filename(&props)) {
/**
   Notice that each character is realized at most twice (once positive and once
   negative) and that each species can be declared null at most once.

   Therefore each partial solution con contain at most 2m+n statuses.
*/
                state_s states[temp->num_species + 2 * temp->num_characters];
                copy_state(states, temp);
                assert(outf != NULL);
                if (exhaustive_search(states, alphabetic)) {
                        for (uint32_t level=0; (states + level)->num_species > 0; level++)
                                fprintf(outf, "%d ", (states + level)->realized_char);
                        fprintf(outf, "\n");
                } else {
                        fprintf(outf, "Not found\n");
                }
        }
        fclose(outf);
        cmdline_parser_free(&args_info);
        CHECK_LEAKS();
        return 0;
}
