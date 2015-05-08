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
        if (log_debug("next_character: pre"))
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
                if (log_debug("next_character: post"))
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
        fewest_characters(stp);
        log_state(stp);
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
        state_s *current = states + level;
        if (log_debug("Called next_node"))
                log_state(current);

        for (uint32_t i = 0; i <= level; i++)
                log_debug("malloc stack level %d %p", i, &((states+i)->red_black));
        if (level_completed(current)) {
                log_debug("LEVEL. Backtrack to level: %d", level - 1);
                return (level - 1);
        }
        if (log_debug("Inside next_node"))
                log_state(current);
        current->realize = next_character(current);
        state_s *next = states + (level + 1);
        log_debug("realizing %d %p %p", level, next, current);
        bool status = realize_character(next, current);
        if (status) {
                log_debug("LEVEL. Go to level: %d", level + 1);
                init_node(next, get_characters_to_realize);
                return (level + 1);
        }
        /***********************************************/
        /* The next solution is not feasible        */
        /***********************************************/
        log_debug("LEVEL. Stay at level: %d", level);
        return (level);
}

bool
exhaustive_search(state_s *states, strategy_fn strategy, uint32_t max_depth) {
        init_node(states, strategy);
        for(uint32_t level = 0; level != -1; level = next_node(states, level, strategy)) {
                cleanup(states + level);
                if ((states + level)->num_species == 0) {
                        return true;
                }
                assert(level <= max_depth);
        }
        return false;
}
