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

   The \c root_state gives the current state for each original species. The
   species that are not in the current instance have value \c UINT32_MAX or \c
   -1.

   \c species and \c characters are the list of current species and characters
   respectively.
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

/**
   \struct operation
   \brief an operation that has been, or can be, applied to an instance.

   The \c type can take the following values:

   * \c 0 => no operation
   * \c 1 => realized a positive character
   * \c 2 => realized a negative character, a character has been freed
   * \c 3 => null characters/species have been removed

   \c removed_species_list and \c removed_characters_list are lists of original
   species and characters, while \c removed_red_black_list and \c
   removed_conflict_list are lists of vertices of the red-black and conflict graphs.
*/
typedef struct operation {
        uint32_t type;
        GSList *removed_species_list;
        GSList *removed_characters_list;
        GSList *removed_red_black_list;
        GSList *removed_conflict_list;
} operation;

/**
   \brief managing operations: \c new_operation \c init_operation \c
   destroy_operation
*/
operation *
new_operation(void);

void
init_operation(operation *op);

void
destroy_operation(operation *op);

void
free_operation(operation *op);

void
copy_operation(operation* dst, const operation* src);
/**
   \param filename: the corresponding file contains an input matrix
   \return pp: the corresponding instance
*/
pp_instance
read_instance_from_filename(const char *filename);

/**
   \param src: instance
   \param character: the character \b name to be realized
   \param op: a pointer to an operation. It must have been already initialized

   \return the instance after the realization of \c character

   The memory necessary to store the newly created instance is automatically
   allocated. It must be freed with \c destroy_instance after it has been used.
*/
pp_instance
realize_character(const pp_instance src, const uint32_t character, operation *op);


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

   It consists of \c operation, \c instance, \c realized_char and \c
   tried_chars which are respectively the operation to apply to the current
   instance (\c instance), the character to realize and the list of characters
   that we have previously tried to realize (without success)
*/
typedef struct state_s {
        operation *operation;
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
pp_instance
instance_cleanup(const pp_instance src, operation *op);

/**
   \brief copy a state

   The destination must have already been allocated
*/
void copy_state(state_s* dst, const state_s* src);


/**
   \brief read a state (instance, operation) from a
   file
*/
state_s*
read_state(const char* filename);

/**
   \brief write a state (instance, operation) to a
   file
*/
void
write_state(const char* filename, state_s* stp);

/**
   \brief computes a list of the characters that can be realized

   \param pointer to the state
*/
GSList* characters_list(state_s * stp);
