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
   The strategy is a function that take as a parameter a pointer to the last
   valid state (or \c NULL at the very first step), analyzes such state to
   determine the order of realization of the characters.

   In other words, it computes in which order we try to realize the characters
   at the current node of the decision tree
*/
typedef GSList* (*strategy_fn)(states_s *stp);

/**
   \brief visits the entire tree of the possible completions

   \param inst: the initial instance
*/

GSList *
exhaustive_search(state_s **states, const pp_instance inst);
