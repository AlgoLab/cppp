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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <err.h>
#include <igraph.h>
#include <inttypes.h>

#define SPECIES 0
#define BLACK 1
#define RED   2

/**
   \struct pp_instance
   \brief the data structure to store a single instance

   The \c matrix field can be \c NULL, if we are not interested in the matrix
   any more.

   The fields \c species_label and \c character_label allow to quickly identify
   the vertex of a red-black (or conflict) graph with a certain label
*/
typedef struct pp_instance {
    uint32_t num_species;
    uint32_t num_characters;
    igraph_t *red_black;
    igraph_t *conflict;
    uint8_t  *matrix;
    uint32_t *species_label;
    uint32_t *character_label;
    uint32_t *conflict_label;
} pp_instance;

/**
   \param filename: the corresponding file contains an input matrix
   \return pp: the corresponding instance
*/
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
realize_character(const pp_instance *src, const uint32_t character);

/**
   \param inst: instance

   Deallocates all memory used to store \c inst.
*/
void
destroy_instance(pp_instance *instp);

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


/**
   Read an entry of an instance matrix

   \param matrix
   \param species: goes from 0 to the number of species - 1
   \param character: goes from 0 to the number of characters - 1
*/
uint8_t
matrix_get_value(pp_instance *instp, uint32_t species, uint32_t character);

/**
   Set the value of an entry of an instance matrix

   \param matrix
   \param species: goes from 0 to the number of species - 1
   \param character: goes from 0 to the number of characters - 1
   \param the value to write
*/
void
matrix_set_value(pp_instance *instp, uint32_t species, uint32_t character, uint8_t value);

/**
   \brief deallocates all memory used to store an instance

   \param pointer to the instance
*/
void
destroy_instance(pp_instance *instp);
