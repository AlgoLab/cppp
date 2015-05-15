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

#include "decision_tree.h"

/**
   \c no_sibling_p returns \c true iff there are no other characters
   to  try at the current level
*/
static bool
level_completed(const state_s * stp) {
        return (stp->character_queue_size == 0);
}

/**
   modifies the current state \c stp so that the next available
   character is computed and the list \c tried_characters and \c
   character_queue are updated
*/
static uint32_t
next_character(state_s *stp) {
        log_debug("next_character: stp=%p", stp);
        log_state_lists(stp);
        if (stp->character_queue_size > 0 ) {
                /* we have found a character to try */
                uint32_t c = stp->character_queue[0];
                stp->tried_characters[stp->tried_characters_size] = c;
                stp->tried_characters_size += 1;
                stp->character_queue_size -= 1;
                for (uint32_t i = 0; i < stp->character_queue_size; i++)
                        stp->character_queue[i] = stp->character_queue[i+1];
                log_debug("next_character: %d", c);
                log_debug("next_character: end");
                log_state_lists(stp);
                return c;
        }
        return -1;
}

/**
   \brief set up the new node of the decision tree
*/
static void
init_node(state_s *stp, strategy_fn get_characters_to_realize) {
        log_debug("init_node");
        stp->tried_characters = GC_MALLOC_ATOMIC(stp->num_characters_orig * sizeof(uint32_t));
        assert(stp->tried_characters != NULL);
        stp->character_queue = GC_MALLOC_ATOMIC(stp->num_characters_orig * sizeof(uint32_t));
        assert(stp->character_queue != NULL);
        stp->tried_characters_size = 0;
        smallest_component(stp);
        log_state(stp);
        log_debug("init_node:end");
}

/**
   \brief computes the next node of the decision tree

   \param states: the set of states, since the decision tree can move to the
   next level or to get back to the previous level
   \param level: the current level
   \param strategy_fn: a pointer to the function encoding the order of the
   characters that we will try in the current level

   \return the new level. It can differ from the input level at most by 1.

   We keep track of the lists of \c tried_character (that is the characters that
   we have already tried to realized in the current level) and of \c
   character_queue (that is the characters left to try) to determine at which
   stage we are. More precisely, if \c tried_character is \c NULL, we are at the
   beginning, and if character_queue is \c NULL we are at the end.
*/
static uint32_t
next_node(state_s *states, uint32_t level, strategy_fn get_characters_to_realize) {
        log_debug("next_node: level=%d", level);
        state_s *current = states + level;
        log_debug("Called next_node");
        log_state(current);

        assert(current->backtrack_level < level || level == 0);
        if (level_completed(current)) {
                log_debug("next_node: end. LEVEL. Backtrack to level: %d from %d", current->backtrack_level, level);
                return (level - 1);
        }
        log_debug("Inside next_node");
        current->realize = next_character(current);
        assert(current->realize <= current->num_characters_orig);
        state_s *next = states + (level + 1);
        log_debug("next_node: realizing level=%d current->realize=%d %p %p", level, current->realize, next, current);
        bool status = realize_character(next, current);
        if (status) {
/* Check if we have resolved the whole instance */
                if (next->num_species == 0) {
                        log_debug("next_node: Solution found");
                        return(level + 1);
                }
                log_debug("next_node: LEVEL. Go to level: %d", level + 1);
                init_node(next, get_characters_to_realize);
                next-> backtrack_level = level;
                if (level_completed(current)) {
/* In this case we have resolved a connected component of the
   red-black graph. Find the level of the decision tree where we have
   started resolving such connected component.

   It is equal to the topmost level whose current_component has
   includes the original species and characters that are not current.
*/
                        bool current_species_characters[current->red_black->num_vertices];
                        memset(current_species_characters, 0, current->red_black->num_vertices * sizeof(current_species_characters[0]));
                        for (uint32_t s = 0; s < current->num_species; s++)
                                current_species_characters[s] = current->species[s];
                        for (uint32_t c = 0; c < current->num_characters; c++)
                                current_species_characters[c + current->num_species] = current->characters[c];

                        uint32_t blevel = level - 1;
                        while (blevel <= level && bool_array_includes(current_species_characters, (states+blevel)->current_component, current->red_black->num_vertices)) {
                                current->backtrack_level = blevel;
                                blevel -= 1;
                        }
                }
                log_debug("next_node: end. LEVEL. Move to level: %d", level + 1);
                return (level + 1);
        }
/***********************************************/
/* The next solution is not feasible        */
/***********************************************/
        log_debug("next_node: end. LEVEL. Stay at level: %d", level);
        return (level);
}

bool
exhaustive_search(state_s *states, strategy_fn strategy, uint32_t max_depth) {
        cleanup(states + 0);
        update_connected_components(states + 0);
        init_node(states + 0, strategy);
        (states + 0)->backtrack_level = -1;
        for(uint32_t level = 0; level != -1; level = next_node(states, level, strategy)) {
                if ((states + level)->num_species == 0) {
                        log_debug("exhaustive_search: solution found");
                        return true;
                }
                assert(level <= max_depth);
        }
        log_debug("exhaustive_search: solution not found");
        return false;
}
