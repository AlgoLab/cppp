/*
  Copyright (C) 2014 by Gianluca Della Vedova


  You can redistribute this file and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Box is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this file
  If not, see <http://www.gnu.org/licenses/>.
*/

/** \mainpage


    Notice that an instance of the perfect phylogeny problem can be represented
    as a matrix or, as in our case, as two graphs:

    * red-black graph, where the vertices are species and characters. A black
    edge \f$(s,c)\f$ means that \f$ M(s,c)=1 \f$ and a red edge means that \f$M(s,c)=0\f$,
    * conflict graph, whose vertices are the characters and two characters are
    adjacent iff they induce the four gametes
*/


/**
   @file perfect_phylogeny.h
   @brief This file contains all public functions that can be useful for dealing with
   perfect phylogenies
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <libgen.h>     /*  for dirname(3) */
#undef basename         /*  (snide comment about libgen.h removed) */
#include <string.h>     /*  for basename(3) (GNU version) and strcmp(3) */
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <err.h>
#include "graph.h"
#include <inttypes.h>
#include <glib.h>
#include <gc.h>
#include "logging.h"

#define SPECIES 0
#define BLACK 1
#define RED   2
#define MAX_COLOR 2
/**
   \struct state_s
   \brief an instance and the
   possible completions that have
   been already tried

   It stores everything that is necessary to construct the final phylogeny and
   to determine the next step of the strategy.

   It consists of \c instance, \c realized_char, \c
   tried_chars, and \c character_queue which are respectively the current
   instance, the character to realize and the list of characters
   that we have previously tried to realize and the candidate characters left.

   Notice that the last character in \c tried_characters is equal to \c realized_char

   The \c matrix field can be \c NULL, if we are not interested in the matrix
   any more.

   The \c current_states gives the current state for each original species.

   \c species and \c characters are two arrays whose values are 1 for the actual species and characters
   respectively.

   \c operation is the code for the most recent operation:
   0 => failure
   1 => realize an inactive character
   2 => realize an active character

   the \c color of each character encodes if it is active or not.
   The possible values are:
   BLACK => the character is inactive
   RED   => the character is active
*/
typedef struct state_s {
        uint32_t realize;
        uint32_t num_species;
        uint32_t num_characters;
        uint32_t num_species_orig;
        uint32_t num_characters_orig;
        graph_s *red_black;
        graph_s *conflict;
        uint32_t *matrix;
        uint32_t *current_states;
        uint32_t *species;
        uint32_t *characters;
        uint8_t  *colors;
        uint32_t operation;
        uint32_t  *tried_characters;
        uint32_t  *character_queue;
        uint32_t tried_characters_size;
        uint32_t character_queue_size;
} state_s;

/**
   \struct array_s
   \brief a simple struct consisting of an array and its length
*/
typedef struct array_s {
        uint32_t *array;
        uint32_t size;
} array_s;

/**
   \brief managing states:
   \c init_state is used only at the very beginning and zeroes the
   entire data structure,
   \c reset_state allocates the whole structure, taking the
   number of original species and characters from the structure.
*/
void init_state(state_s *stp, uint32_t nspecies, uint32_t nchars);

void
reset_state(state_s * stp);

void
free_state(state_s *stp);

/**
   \brief check if a state is internally consistent

   \return \c true if all check have been passed
*/
bool check_state(const state_s* stp);

/**
   \brief simplify the current instance, if possible

   \param instance to be simplified
   \return simplified instance

   The goal is to obtain a new instance that has the same solutions as the
   original instance. More precisely:

   * we remove all isolated vertices of the red-black graph
   * we remove duplicated characters
   * we remove duplicated species

   Notice that at each function, at most one of those operations is performed,
   therefore it is necessary to include this function in a \c while loop to
   completely simplify the instance
*/
void cleanup(state_s *src);

/**
   \brief copy a state

   The \c characters_queue and the \c tried_characters are not copied.
   To copy also those fields, use the \c full_copy_state function.
   The destination must have already been allocated

*/
void
copy_state(state_s* dst, const state_s* src);

void
full_copy_state(state_s* dst, const state_s* src);


/**
   \brief read a state from a file
*/
void
read_state(const char* filename, state_s* stp);

/**
   \brief write a state to a file
*/
void
write_state(const char* filename, state_s* stp);

/**
   \brief computes a list of the characters that can be realized

   \param pointer to the state
*/
uint32_t
characters_list(state_s * stp, uint32_t *array);

/**
   \brief delete a species from the set of current species
*/
void delete_species(state_s* stp, uint32_t s);

/**
   \brief delete a character from the set of current character
*/
void delete_character(state_s* stp, uint32_t c);

/**
   \struct data common to all instances in a file
*/
typedef struct instances_schema_s {
        FILE* file;
        uint32_t num_species;
        uint32_t num_characters;
        char* filename;
} instances_schema_s;

/* /\** */
/*    \brief open the file containing the instances */
/*    \param the filename */
/* *\/ */
/* instances_schema_s open_instance_file(const char *); */

/**
   \brief read another instance from file, if possible. Returns \c
   true if the instance has been read correctly.

   \param the filename and a pointer to the state that will contain
   the data read from the file


*/
bool
read_instance_from_filename(instances_schema_s* global_props, state_s* stp);

/**
   \param character: the source state \c src and the outcome \c dst of
   the realization. The character to realize is \c src->realize

   \return \c true if the realization has been successful

*/
bool realize_character(state_s* dst, const state_s* src);


/**
   \param inst: state_s
   \return the red-black graph associated to the input instance
*/
graph_s *
get_red_black_graph(const state_s *stp);

/**
   \param inst: state_s
   \return the conflict graph associated to the input instance
*/
graph_s *
get_conflict_graph(const state_s *stp);

/**
   Print a dump of a state
   \param stp: pointer to state_s
*/
void log_state(const state_s* stp);
void log_state_lists(const state_s* stp);
void log_state_graphs(const state_s* stp);
