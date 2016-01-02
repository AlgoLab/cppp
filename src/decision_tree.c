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
   \brief prints a dump of the sequence of characters realized

*/
static void log_decisions(const state_s* arr_stp, const uint32_t max_depth) {
#ifdef DEBUG
        log_debug("log_decisions");
        fprintf(stderr, "=========BEGIN DECISIONS===============\n");
        for (uint32_t l = 0; l <= max_depth; l++)
                fprintf(stderr, "level=%4d Character=%d\n", l, (arr_stp+l)->realize);
        fprintf(stderr, "=========END DECISIONS=================\n");
#endif
}



/**
   \c no_sibling_p returns \c true iff there are no other characters
   to try at the current level
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
        stp->tried_characters_size = 0;
        smallest_component(stp);
        log_state(stp);
        log_debug("init_node:end");
}

/**
   \brief tests if the states \c root and \c leaf are first and the
   last of portion of states solving a connected component of the
   red-black graph of the state \c root.

   The goal is to simulate the fact that each connected component of a
   red-black graph can be solved independently.

   The set of characters at the state root that are not in the state
   leaf are exactly the characters fully realized in the portion
   between root and leaf. We have to check if those states are exactly
   those in the \c current_component at state root.
*/
static bool
component_borders(state_s* states, uint32_t root_level, uint32_t leaf_level) {
        state_s* root = states + root_level;
        state_s* leaf = states + leaf_level;
        bool* solved = bool_array_difference(root->characters, leaf->characters, root->num_characters_orig);
        bool found = bool_array_equal(solved, (root->current_component) + root->num_species_orig, root->num_characters_orig);
        if (!found)
                return false;
        for (uint32_t l = root_level + 1; l <= leaf_level; l++)
                if (!bool_array_includes(root->current_component , (states + l)->current_component, root->red_black->num_vertices))
                        return false;
        return true;
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
        log_state(current);
        log_decisions(states, level);

        if (level_completed(current)) {
                /* it is not possible to extend the solution. We have
                   to backtrack */
                log_debug("next_node: end. LEVEL. Backtrack to level: %d from %d", current->backtrack_level, level);
                return (current->backtrack_level);
        }
        log_debug("Inside next_node");
        current->realize = next_character(current);
        assert(current->realize <= current->num_characters_orig);
        state_s *next = states + (level + 1);
        log_debug("next_node: realizing level=%d current->realize=%d %p %p", level, current->realize, next, current);
        bool status = realize_character(next, current);
        log_debug("next_node: result of realizing level=%d current->realize=%d outcome=%d", level, current->realize, status);
        if (status) {
                /* The realization has been successful.
                   First check if we have resolved the whole instance */
                if (next->num_species == 0) {
                        log_debug("next_node: Solution found");
                        return(level + 1);
                }

                /* Since we had realized a character, we move to a
                   deeper level of the decision tree. */
                log_debug("next_node: LEVEL. Go to level: %d", level + 1);
                init_node(next, get_characters_to_realize);

                /* Since the realization of the negated characters are forced, we backtrack to the lowest level of the decision tree where the operation is the
                   realization of an inactive character.

                   In fact, this implies that we permute over all realization of inactive characters, instead of the naive permutation of all possible characters.
                */
                for (next->backtrack_level = level; (states + next->backtrack_level)->operation != 1; next->backtrack_level--) ;

                if (level_completed(current)) {
                        log_debug("next_node: connected component completed");
                        /* In this case we have resolved a connected component of the red-black graph. Find the level of the decision tree where we have started
                           resolving such connected component.

                           It is equal to the topmost level whose current_component includes the original species and all characters that are not current.
                        */
                        for (uint32_t blevel = 0; blevel < level; blevel++)
                                if (component_borders(states, blevel, level + 1)) {
                                        next->backtrack_level = blevel - 1;
                                        log_decisions(states, level);
                                        log_debug("Preparing backtrack to level %d from %d (level=%d)", blevel - 1, level + 1, level);
                                        for (uint32_t l = blevel; l <= level; l++) {
                                                log_debug("Level=%d (%d-%d)", l, blevel, level);
                                                log_array_bool("current_component", (states + l)->current_component, (states + blevel)->red_black->num_vertices);
                                                log_array_bool("characters", (states + l)->characters, (states + blevel)->num_characters_orig);
                                        }
                                        log_debug("Next state");
                                        log_state(next);
                                        log_debug("Backtracked state");
                                        log_state(states + blevel);
                                        break;
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
                log_debug("exhaustive_search: level %d", level);
                log_state(states + level);
                assert(level <= max_depth);
                if ((states + level)->num_species == 0) {
                        log_debug("exhaustive_search: solution found");
                        return true;
                }
        }
        log_debug("exhaustive_search: solution not found");
        return false;
}
