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
#include "decision_tree.h"
#include "gc/leak_detector.h"

static uint32_t
alphabetic(state_s *stp, uint32_t *arr) {
        return (characters_list(stp, arr));
}

int main(int argc, char **argv) {
        static struct gengetopt_args_info args_info;
        assert(cmdline_parser(argc, argv, &args_info) == 0);
        assert(args_info.inputs_num >= 1);
        start_logging(args_info);
        FILE* outf = fopen(args_info.output_arg, "w");

        instances_schema_s props = {
                .file = NULL,
                .filename = args_info.inputs[0]
        };
        state_s temp;
        for (;read_instance_from_filename(&props, &temp);) {
/**
   Notice that each character is realized at most twice (once positive and once
   negative) and that each species can be declared null at most once.

   Therefore each partial solution con contain at most 2m+n states.
*/
                state_s states[temp.num_species + 2 * temp.num_characters];
                /*
                  make all num_species equal to zero, so that the
                  flushing step does not try to free unallocated and
                  uninitialized memory
                */
                for (uint32_t level=0; level<temp.num_species + 2 * temp.num_characters; level++)
                        (states + level)->num_species = 0;

                copy_state(states, &temp);
                free_state(&temp);
                assert(outf != NULL);
                if (exhaustive_search(states, alphabetic, states[0].num_species + 2 * states[0].num_characters)) {
                        for (uint32_t level=0; (states + level)->num_species > 0; level++) {
                                fprintf(outf, "%d ", (states + level)->realize);
                        }
                        fprintf(outf, "\n");
                } else {
                        fprintf(outf, "Not found\n");
                }
                for (uint32_t level=0; (states + level)->num_species > 0; level++) {
                        log_debug("malloc flushing level %d %p", level, &((states + level)->red_black));
                        free_state(states + level);
                }
        }
        fclose(outf);
        cmdline_parser_free(&args_info);
        return 0;
}
