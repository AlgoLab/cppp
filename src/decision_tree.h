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

/**
   The strategy is a function that take as a parameter a pointer to a new
   state, analyzes such state to which characters can be realized and
   their order.

   In other words, it computes in which order we try to realize the characters
   at the current node of the decision tree
*/
typedef uint32_t (*strategy_fn)(state_s *stp, uint32_t *chars);

/**
   \brief visits the entire tree of the possible completions

   \param states: an array of states. In \c states[0] is stored the initial
   instance
   \param strategy: the callback function that determines the order according to
   which all characters are tried
   \param max_depth: maximum depth of the search tree

   returns \c true iff a solution is found
*/

bool
exhaustive_search(state_s *states, strategy_fn strategy, uint32_t max_depth);
