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
#include "perfect_phylogeny.h"
#include "logging.h"



int main(int argc, char **argv) {
    static struct gengetopt_args_info args_info;
    assert(cmdline_parser (argc, argv, &args_info) == 0);
    assert(args_info.inputs_num >= 1);
    start_logging(args_info.quiet_given, args_info.log_given, args_info.log_arg);

    igraph_i_set_attribute_table(&igraph_cattribute_table);
    pp_instance temp = read_instance_from_filename(args_info.inputs[0]);

/*
  The maximum depth of a phylogeny is 2n, therefore we can have at most 2n
  red-black and conflict graphs.
*/
    pp_instance instances[2*temp.num_species];

    uint32_t level=0;
    instances[0] = temp;
    temp = instances[0];

    while(level > 0) {
        ;
    }

    for(uint8_t i=0; i <= level; i++)
        destroy_instance(instances+i);

    end_logging();
    cmdline_parser_free(&args_info);
    return 0;
}
