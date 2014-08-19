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


/**
   @file perfect_phylogeny.c
   @brief Implementation of @c perfect_phylogeny.h

*/
#include "perfect_phylogeny.h"

/**
   \param src
   \return dest: a deep copy of src, ie a new instance with same values as \c src
*/
static pp_instance
copy_instance(const pp_instance *src) {
    igraph_t *grb = malloc(sizeof(igraph_t));
    assert(grb != NULL);
    igraph_t *gcf = malloc(sizeof(igraph_t));
    assert(gcf != NULL);
    igraph_copy(grb, src->red_black);
    igraph_copy(gcf, src->conflict);

    pp_instance dest = {.num_species = src->num_species,
                        .num_characters = src->num_characters,
                        .red_black = grb,
                        .conflict = gcf
    };
    return dest;
}

pp_instance
realize_character(const pp_instance *src, const uint32_t character) {
    pp_instance dest = copy_instance(src);
//TODO
    return dest;
}

/* static int */
/* conflict_graph_init(igraph_t *g, igraph_t *red_black, uint32_t num_species, uint32_t num_characters) { */
/*     /\* for(uint32_t c1=n; c1<n+m; c1++) *\/ */
/*     /\*     for(uint32_t c2=c1+1; c2<n+m; c2++) { *\/ */
/*     /\*         igraph_vector_ptr_t n1, n2; *\/ */
/*     /\*         igraph_neighborhood(red_black, n1, c1, 1, 0); *\/ */
/*     /\*         igraph_neighborhood(red_black, n2, c2, 1, 0); *\/ */
/*     /\*     } *\/ */

/*     return 0; */
/* } */

/**
   \param matrix: instance matrix
   \param n: number of species
   \param m: number of characters

   \return the red-black graph corresponding to the input matrix
*/
static igraph_t *
matrix2redblack(uint8_t *matrix, uint32_t n, uint32_t m) {
    igraph_t *rb = malloc(sizeof(igraph_t *));
    assert(rb != NULL);
    igraph_empty_attrs(rb, n+m, 0, 0);

    // the first n ids correspond to species
    // the ids in n..n+m-1 correspond to characters
    // color attribute is SPECIES if the vertex is a species
    for(uint32_t s=0; s<n; s++) {
        SETVAN(rb, "original", s, s);
        SETVAN(rb, "color", s, SPECIES);
    }
    for(uint32_t c=0; c<m; c++) {
        SETVAN(rb, "original", c+n, c);
        SETVAN(rb, "color", c+n, BLACK);
    }
    for (uint32_t s=0; s<n; s++)
        for (uint32_t c=0; c<m; c++)
            if (matrix[s*m+c] == 1)
                igraph_add_edge(rb, s, c);
    return rb;
}

static igraph_t *
matrix2conflict(uint8_t *matrix, uint32_t num_species, uint32_t num_characters) {
    igraph_t *g = malloc(sizeof(igraph_t *));
    assert(g != NULL);
    igraph_empty(g, num_characters, IGRAPH_UNDIRECTED);
    // TODO
    return g;
}

pp_instance
read_instance_from_filename(const char *filename) {
    FILE* file;
    file = fopen(filename, "r");
    assert(file != NULL);
    assert(!feof(file));
    uint32_t num_species, num_characters;

    assert(fscanf(file, "%"SCNu32" %"SCNu32, &num_species, &num_characters) != EOF);
    uint8_t matrix[num_species*num_characters];
    for(uint32_t s=0; s < num_species; s++)
        for(uint32_t c=0; c < num_characters; c++) {
            uint8_t x;
            assert(fscanf(file, "%"SCNu8, &x) != EOF);
            matrix[c+s*num_species]=x;
        }

    igraph_t *grb = matrix2redblack(matrix, num_species, num_characters);
    igraph_t *gcf = matrix2conflict(matrix, num_species, num_characters);

    pp_instance inst = {.num_species = num_species,
                        .num_characters = num_characters,
                        .red_black = grb,
                        .conflict = gcf
    };
    return inst;
}

pp_instance
read_instance_from_filename(const char *filename) {
    FILE* file;
    file = fopen(filename, "r");
    assert(file != NULL);
    assert(!feof(file));
    uint32_t num_species, num_characters;

    assert(fscanf(file, "%"SCNu32" %"SCNu32, &num_species, &num_characters) != EOF);
    uint8_t matrix[num_species*num_characters];
    for(uint32_t s=0; s < num_species; s++)
        for(uint32_t c=0; c < num_characters; c++) {
            uint8_t x;
            assert(fscanf(file, "%"SCNu8, &x) != EOF);
            matrix[c+s*num_species]=x;
        }

    igraph_t *grb = matrix2redblack(matrix, num_species, num_characters);
    igraph_t *gcf = matrix2conflict(matrix, num_species, num_characters);

    pp_instance inst = {.num_species = num_species,
                        .num_characters = num_characters,
                        .red_black = grb,
                        .conflict = gcf
    };
    return inst;
}



/*********************************************************
 simplify the red-black graph whenever possible.

The graph at the end of the function has the same solution as at function
invocation
*********************************************************/
/* static int */
/* red_black_graph_cleanup(igraph_t *rb, uint32_t num_species, uint32_t num_characters) { */
/*     // remove isolated species and characters */
/*     igraph_vector_t clusters, sizes, isolated; */
/*     igraph_vector_init(&clusters, 0); */
/*     igraph_vector_init(&sizes, 0); */
/*     igraph_vector_init(&isolated, 0); */
/*     /\* igraph_clusters(rb, clusters, sizes, 0, 0); *\/ */

/*     /\* for (uint32_t i=0; i<igraph_vector_size(clusters); i++) *\/ */
/*     /\*     if (igraph_vector_e(sizes, i) == 1) *\/ */
/*     /\*         /\\* *\/ */
/*     /\*           when the cluster contains only one vertex, the cluster id is also *\/ */
/*     /\*           the vertex id *\/ */
/*     /\*         *\\/ *\/ */
/*     /\*         igraph_vector_push_back(isolated, igraph_vector_e(clusters, i)); *\/ */
/*     /\* igraph_delete_vertices(rb, isolated); *\/ */

/*     /\* igraph_vector_delete(clusters); *\/ */
/*     /\* igraph_vector_delete(sizes); *\/ */
/*     /\* igraph_vector_delete(isolated); *\/ */

/*     igraph_vector_destroy(&clusters); */
/*     igraph_vector_destroy(&sizes); */
/*     igraph_vector_destroy(&isolated); */
/* // TODO: remove duplicated species */

/*     // TODO: remove duplicated characters */
/*     return 0; */
/* } */

void
destroy_instance(pp_instance *inst) {
    // TODO
}

igraph_t *
get_red_black_graph(const pp_instance *inst) {
    // TODO
    return NULL;
}

igraph_t *
get_conflict_graph(const pp_instance *inst) {
    // TODO
    return NULL;
}

matrix_s *
get_matrix(pp_instance *instance) {
    // TODO
    return NULL;
}

uint8_t
matrix_get_value(matrix_s *matrix, uint32_t species, uint32_t character) {
    // TODO
    return 0;
}

void
matrix_set_value(matrix_s *matrix, uint32_t species, uint32_t character, uint8_t value) {
    // TODO
}

void
destroy_matrix(matrix_s *matrix) {
    // TODO
}
