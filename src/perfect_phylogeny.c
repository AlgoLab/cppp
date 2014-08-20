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
#ifdef TEST_EVERYTHING
#include <check.h>
#include <stdlib.h>
#endif
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

   In a red-black graph, the first \c instp->num_species ids correspond to species,
   while the ids larger or equal to instp->num_species correspond to characters.
   Notice that the label id must be conserved when modifying the graph (i.e.
   realizing a character).

   color attribute is \c SPECIES if the vertex is a species, otherwise it is \c BLACK
   or \c RED (at the beginning, there can only be \c BLACK edges).

*/
static igraph_t *
matrix2redblack(pp_instance *instp) {
    igraph_t *rb = malloc(sizeof(igraph_t));
    assert(rb != NULL);
    igraph_empty_attrs(rb, instp->num_species+instp->num_characters, IGRAPH_UNDIRECTED, 0);

    for(uint32_t s=0; s<instp->num_species; s++) {
        SETVAN(rb, "id", s, s);
        SETVAN(rb, "color", s, SPECIES);
    }
    for(uint32_t c=0; c<instp->num_characters; c++) {
        SETVAN(rb, "id", c+instp->num_species, c);
        SETVAN(rb, "color", c+instp->num_species, BLACK);
    }
    for (uint32_t s=0; s<instp->num_species; s++)
        for (uint32_t c=0; c<instp->num_characters; c++)
            if (matrix_get_value(instp, s, c) == 1)
                igraph_add_edge(rb, s, c+instp->num_species);
    return rb;
}


static igraph_t *
matrix2conflict(pp_instance *instp) {
    igraph_t *g = malloc(sizeof(igraph_t));
    assert(g != NULL);
    igraph_empty(g, instp->num_characters, IGRAPH_UNDIRECTED);
    // TODO
    return g;
}

/**
   \brief some functions to abstract the access to the instance matrix
*/

uint8_t
matrix_get_value(pp_instance *inst, uint32_t species, uint32_t character) {
    return inst->matrix[character + inst->num_characters*species];
}

void
matrix_set_value(pp_instance *inst, uint32_t species, uint32_t character, uint8_t value) {
    inst->matrix[character + inst->num_characters*species] = value;
}

static uint8_t *
init_matrix(uint32_t num_species, uint32_t num_characters) {
    uint8_t *m = calloc(num_species*num_characters, sizeof(uint8_t));
    assert(m != NULL);
    return m;
}


pp_instance
read_instance_from_filename(const char *filename) {
    FILE* file;
    file = fopen(filename, "r");
    assert(file != NULL);
    assert(!feof(file));
    uint32_t num_species, num_characters;

    assert(fscanf(file, "%"SCNu32" %"SCNu32, &num_species, &num_characters) != EOF);
    pp_instance inst = {.num_species = num_species,
                        .num_characters = num_characters,
                        .matrix = init_matrix(num_species, num_characters)
    };

    for(uint32_t s=0; s < num_species; s++)
        for(uint32_t c=0; c < num_characters; c++) {
            uint8_t x;
            assert(fscanf(file, "%"SCNu8, &x) != EOF);
            matrix_set_value(&inst, s, c, x);
        }

    inst.red_black = matrix2redblack(&inst);
    inst.conflict = matrix2conflict(&inst);

    fclose(file);
    return inst;
}

#ifdef TEST_EVERYTHING
#include <check.h>

#endif

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

#ifdef TEST_EVERYTHING
#define TEST_MATRIX_PP {                                                \
        ck_assert_int_eq(instance.num_species, num_species);            \
        ck_assert_int_eq(instance.num_characters, num_characters);      \
        for (uint8_t i=0; i<instance.num_species; i++)                  \
            for (uint8_t j=0; j<instance.num_characters; j++)    {      \
                ck_assert_int_eq(matrix_get_value(&instance, i, j), data[i][j]); \
                igraph_integer_t eid;                                   \
                igraph_get_eid(instance.red_black, &eid, i, j+instance.num_species, 0, 0); \
                if (data[i][j] == 1)									\
                    ck_assert_int_ge(eid, 0);							\
                else													\
                    ck_assert_int_lt(eid, 0);                           \
            }                                                           \
    }


START_TEST(test_read_instance_from_filename_1) {
    const uint8_t num_species = 4;
    const uint8_t num_characters = 4;
    const uint8_t data[4][4] = {
        {0, 0, 1, 1},
        {0, 1, 0, 1},
        {1, 0, 1, 0},
        {1, 1, 0, 0}
    };
    pp_instance instance = read_instance_from_filename("tests/input/read/1.txt");
    TEST_MATRIX_PP;
}
    END_TEST

        START_TEST(test_read_instance_from_filename_2) {
    const uint8_t num_species = 6;
    const uint8_t num_characters = 3;
    const uint8_t data[6][3] = {
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 1},
    {1, 0, 0},
    {1, 0, 1},
    {1, 1, 0}
};
    pp_instance instance = read_instance_from_filename("tests/input/read/2.txt");
    TEST_MATRIX_PP;
    //    igraph_write_graph_gml(instance.red_black, stdout, 0, 0);
}
    END_TEST

        static Suite * perfect_phylogeny_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("perfect_phylogeny.c");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_read_instance_from_filename_1);
    tcase_add_test(tc_core, test_read_instance_from_filename_2);
    suite_add_tcase(s, tc_core);

    return s;
}

    int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;
    igraph_i_set_attribute_table(&igraph_cattribute_table);

    s = perfect_phylogeny_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
