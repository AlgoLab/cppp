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
#include <igraph.h>
#include <inttypes.h>
#include <glib.h>
#include <jansson.h>
#include <gc.h>

#define SPECIES 0
#define BLACK 1
#define RED   2

/**
   \struct pp_instance
   \brief the data structure to store a single instance

   The \c matrix field can be \c NULL, if we are not interested in the matrix
   any more.

   The \c current gives the current state for each original species.

   \c species and \c characters are two arrays whose values are 1 for the current species and characters
   respectively.

   \c operation is the code for the most recent operation:
   0 => realize an inactive character
   1 => realize an active character
   -1 => failure
*/
typedef struct pp_instance {
        uint32_t num_species;
        uint32_t num_characters;
        uint32_t num_species_orig;
        uint32_t num_characters_orig;
        igraph_t *red_black;
        igraph_t *conflict;
        uint32_t *matrix;
        uint32_t *current;
        uint32_t *species;
        uint32_t *characters;
        uint32_t operation;
} pp_instance;

/**
   \brief managing instances: \c new_instance \c init_instance \c
   destroy_instance \c free_instance

   \c destroy_instance does not free the instance, while \c free_instance does
*/
pp_instance *
new_instance(void);

/* void */
/* init_instance(pp_instance * instp, uint32_t num_species, uint32_t num_characters); */

void
destroy_instance(pp_instance *instp);

void
free_instance(pp_instance *instp);

void str_instance(const pp_instance* instp, char* str);
/**
   \brief copy an instance

   \param src, dst: pointers to the instances

   The instance must have already been allocated.
*/
void
copy_instance(pp_instance *dst, const pp_instance *src);

pp_instance
read_instance_from_filename(const char *filename);

/**
   \param src: instance
   \param character: the character \b name to be realized

   \return the instance after the realization of \c character

   The memory necessary to store the newly created instance is automatically
   allocated. It must be freed with \c destroy_instance after it has been used.
*/
pp_instance
realize_character(const pp_instance src, const uint32_t character);


/**
   \param inst: instance
   \return the red-black graph associated to the input instance
*/
igraph_t *
get_red_black_graph(const pp_instance *instp);

/**
   \param inst: instance
   \return the conflict graph associated to the input instance
*/
igraph_t *
get_conflict_graph(const pp_instance *instp);


/* /\** */
/*    Read an entry of an instance matrix */

/*    \param matrix */
/*    \param species: goes from 0 to the number of species - 1 */
/*    \param character: goes from 0 to the number of characters - 1 */
/* *\/ */
/* uint32_t */
/* matrix_get_value(pp_instance *instp, uint32_t species, uint32_t character); */

/* /\** */
/*    Set the value of an entry of an instance matrix */

/*    \param matrix */
/*    \param species: goes from 0 to the number of species - 1 */
/*    \param character: goes from 0 to the number of characters - 1 */
/*    \param the value to write */
/* *\/ */
/* void */
/* matrix_set_value(pp_instance *instp, uint32_t species, uint32_t character, uint32_t value); */

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
*/
typedef struct state_s {
        pp_instance *instance;
        uint32_t realized_char;
        GSList *tried_characters;
        GSList *character_queue;
} state_s;

/**
   \brief managing states: \c new_state \c init_state \c
   destroy_state
*/

state_s *
new_state(void);

void
init_state(state_s * stp);

void
destroy_state(state_s *stp);

void
free_state(state_s *stp);

/**
   \brief check if a state is internally consistent

   \return 0 if all check have been passed, otherwise an error code larger than 0.
*/
uint32_t check_state(const state_s* stp);

/**
   \brief creates the initial state corresponding to an instance

   It assumes that the input state \c stp has already been allocated.
   The function updates the state so that it is consistent with the input instance.
*/
void first_state(state_s* stp, pp_instance *instp);


/**
   \brief simplify the instance, if possible

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
void instance_cleanup(pp_instance *src);

/**
   \brief copy a state

   The destination must have already been allocated
*/
void copy_state(state_s* dst, const state_s* src);


/**
   \brief read a state (instance) from a
   file
*/
state_s*
read_state(const char* filename);

/**
   \brief write a state (instance) to a
   file
*/
void
write_state(const char* filename, state_s* stp);

/**
   \brief computes a list of the characters that can be realized

   \param pointer to the state
*/
GSList* characters_list(state_s * stp);

/**
   \brief delete a species from the set of current species from an instance
*/
void delete_species(pp_instance *instp, uint32_t s);

/**
   \brief delete a character from the set of current character from an instance
*/
void delete_character(pp_instance *instp, uint32_t c);
