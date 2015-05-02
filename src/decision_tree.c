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
no_sibling_p(state_s *stp) {
        return (stp->character_queue == NULL);
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
next_node(state_s *states, uint32_t level, strategy_fn level_init) {
        state_s *current = states + level;
        if (log_debug("Called next_node"))
                log_state(current);

        for (uint32_t i = 0; i <= level; i++)
                log_debug("malloc stack level %d %p", i, &((states+i)->red_black));
        if (current->tried_characters == NULL && no_sibling_p(current))
                /* it is the first node of a level */
                current->character_queue = level_init(current);
        if (no_sibling_p(current)) {
                log_debug("LEVEL. Backtrack to level: %d", level - 1);
                free_state(current);
                return (level - 1);
        }
        if (log_debug("Inside next_node"))
                log_state(current);
        current->realize = GPOINTER_TO_INT(g_slist_nth_data(current->character_queue, 0));
        current->character_queue = g_slist_nth(current->character_queue, 1);
        current->tried_characters = g_slist_prepend(current->tried_characters, GINT_TO_POINTER(current->realize));
        log_debug("realizing %d", current->realize);
        state_s *next = current + 1;
        bool status = realize_character(next, current);
        if (status) {
                log_debug("LEVEL. Go to level: %d", level + 1);
                return (level + 1);
        }
        /***********************************************/
        /* The \c next solution is not feasible        */
        /***********************************************/
        free_state(next);
        log_debug("LEVEL. Stay at level: %d", level);
        return (level);
}

bool
exhaustive_search(state_s *states, strategy_fn strategy) {
        uint32_t level = next_node(states, 0, strategy);
        for(;level != -1; level = next_node(states, level, strategy)) {
                cleanup(states + level);
                if ((states + level)->num_species == 0) {
                        return true;
                }
        }
        return false;
}

#ifdef TEST_EVERYTHING
int main(void) {
        return 0;
}
#endif
