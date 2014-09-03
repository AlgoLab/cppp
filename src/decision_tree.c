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

#include "perfect_phylogeny.h"
#include "decision_tree.h"
#include "logging.h"


static bool no_sibling_p(state_s *stp) {
        return (stp->character_queue == NULL);
}

uint32_t next_node(state_s *states, uint32_t level, strategy_fn node_init) {
        state_s *current = states[level];
        if (current->tried_characters == NULL && no_siblings_p(current))
                current->character_queue = node_init(states[level]);
        if (no_sibling_p(current)) {
                destroy_state(current);
                return (level-1);
        }
        current->realized_char = GPOINTER_TO_INT(g_slist_nth_data(current->character_queue, 0));
        current->character_queue = g_slist_nth(current->character_queue, 1);
        pp_instance modified = realize_character(current->realized_char, current->operation);
        if (current->operation->type > 0) {
                states[level + 1] = new_state();
                state_s *next = states[level + 1];
                copy_state(next, current);
                copy_instance(next->instance, current->instance);
                return (level + 1);
        }
        return (level);
}

void
exhaustive_search(state_s *states, const pp_instance inst, strategy_fn strategy) {
        first_state(states+0, &inst);
        uint32_t level = 0;
        while(level != -1) {
                level = next_node(states, level);
                if (g_slist_length(states[level]->instance->num_species) == 0) {
                        printf("Solution found\n");
                }
        }
        printf("END\n");
        for(uint8_t i=0; i <= level; i++)
                destroy_state(states+i);
}
