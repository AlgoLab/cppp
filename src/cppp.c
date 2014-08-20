#include "cppp.h"
#include "perfect_phylogeny.h"




int main(int argc, char *argv[]) {
    // check command line (main) arguments
    igraph_i_set_attribute_table(&igraph_cattribute_table);

    assert(argc >= 2);
    pp_instance temp = read_instance_from_filename(argv[1]);

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

    return 0;
}
