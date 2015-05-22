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

#include "cppp.h"

static uint32_t
alphabetic(state_s *stp, uint32_t *arr) {
        return (characters_list(stp, arr));
}

int main(int argc, char **argv) {
        static struct gengetopt_args_info args_info;
        int cmd_status = cmdline_parser(argc, argv, &args_info);
        if (cmd_status != 0)
                error(4, 0, "Could not parse the command line\n");
        if (args_info.inputs_num < 1)
                error(5, 0, "There is no input matrix to analyze\n");
        start_logging(args_info);
        log_debug("cppp: start");
        FILE* outf = fopen(args_info.output_arg, "w");

        instances_schema_s props = {
                .file = NULL,
                .filename = args_info.inputs[0]
        };
        state_s temp;
        while (read_instance_from_filename(&props, &temp)) {
/**
   Notice that each character is realized at most twice (once positive and once
   negative) and that each species can be declared null at most once.

   Therefore each partial solution con contain at most 2m+n states.
*/
                uint32_t max_depth = temp.num_species + 2 * temp.num_characters + 1;
                state_s states[max_depth];
                for (uint32_t level=0; level <= max_depth; level++) {
                        log_debug("State #%d = %p", level, states + level);
                        check_state(&temp);
                        log_debug("State #%d (max %d) = %p", level, maxdepth, states + level);
                        init_state(states + level, temp.num_species_orig, temp.num_characters_orig);
                        log_debug("State #%d (max %d) = %p", level, maxdepth, states + level);
                        check_state(&temp);
                        log_debug("State #%d (max %d) = %p", level, maxdepth, states + level);
                        log_debug("Initialized level %d", level);
                        log_state(states + level);
                        check_state(&temp);
                        check_state(states + level);
                        log_debug("check temp");
                        check_state(&temp);
                }
                log_debug("States initialized");
                check_state(&temp);

                copy_state(states + 0, &temp);
                if (outf == NULL)
                        error(6, 0, "Input file ended prematurely\n");
                if (exhaustive_search(states, alphabetic, states[0].num_species + 2 * states[0].num_characters)) {
                        log_debug("Writing solution");
                        for (uint32_t level=0; (states + level)->num_species > 0; level++)
                                fprintf(outf, "%d ", (states + level)->realize);
                        fprintf(outf, "\n");
                } else
                        fprintf(outf, "Not found\n");
                log_debug("Instance solved");
        }
        fclose(outf);
        cmdline_parser_free(&args_info);
        log_debug("END");
        return 0;
}
