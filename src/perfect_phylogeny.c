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


static void
init_instance(pp_instance *instp, uint32_t nspecies, uint32_t nchars) {
        assert(instp != NULL);
        if (instp->red_black == NULL) instp->red_black = GC_MALLOC(sizeof(igraph_t));
        if (instp->conflict == NULL)  instp->conflict  = GC_MALLOC(sizeof(igraph_t));
        instp->conflict = GC_MALLOC(sizeof(igraph_t));
        igraph_empty(instp->conflict, nchars, IGRAPH_UNDIRECTED);
        instp->red_black = GC_MALLOC(sizeof(igraph_t));
        igraph_empty_attrs(instp->red_black, nspecies + nchars, IGRAPH_UNDIRECTED, 0);
        instp->species_label = GC_MALLOC(nspecies * sizeof(uint32_t));
        instp->character_label = GC_MALLOC(nchars * sizeof(uint32_t));
        instp->conflict_label = GC_MALLOC(nspecies * sizeof(uint32_t));
        instp->root_state = GC_MALLOC(nchars * sizeof(uint32_t));
}

/**
   \brief some functions to abstract the access to the instance matrix
*/

static uint32_t
matrix_get_value(pp_instance *inst, uint32_t species, uint32_t character) {
        return inst->matrix[character + inst->num_characters*species];
}

static void
matrix_set_value(pp_instance *inst, uint32_t species, uint32_t character, uint32_t value) {
        inst->matrix[character + inst->num_characters*species] = value;
}

/**
   \param instance

   Updates an instance by computing the red-black and the conflict graphs
   associated to a given matrix.

   The input instance must have the \c matrix \c num_species and \c
   num_characters fields already filled in. All other fields are computed by
   this function.

   In a red-black graph, the first \c instp->num_species ids correspond to species,
   while the ids larger or equal to instp->num_species correspond to characters.
   Notice that the label id must be conserved when modifying the graph (i.e.
   realizing a character).

   color attribute is \c SPECIES if the vertex is a species, otherwise it is \c BLACK
   or \c RED (at the beginning, there can only be \c BLACK edges).

*/
static void
matrix2instance(pp_instance *instp) {
        assert(instp->matrix != NULL);
        assert(instp->num_species != 0);
        assert(instp->num_characters != 0);

        init_instance(instp, instp->num_species, instp->num_characters);
        instp->num_species_orig = instp->num_species;
        instp->num_characters_orig = instp->num_characters;

/* We start with the red-black graph */

        for(uint32_t s=0; s<instp->num_species; s++) {
                SETVAN(instp->red_black, "id", s, s);
                instp->species_label[s] = s;
                SETVAN(instp->red_black, "color", s, SPECIES);
                instp->species = g_slist_append(instp->species, GINT_TO_POINTER(s));
        }
        for(uint32_t c=0; c<instp->num_characters; c++) {
                SETVAN(instp->red_black, "id", c+instp->num_species, c);
                instp->character_label[c] = c+instp->num_species;
                instp->conflict_label[c] = c;
                SETVAN(instp->red_black, "color", c+instp->num_species, BLACK);
                instp->characters = g_slist_append(instp->characters, GINT_TO_POINTER(c));
        }
        for (uint32_t s=0; s<instp->num_species; s++)
                for (uint32_t c=0; c<instp->num_characters; c++)
                        if (matrix_get_value(instp, s, c) == 1)
                                igraph_add_edge(instp->red_black, s, c+instp->num_species);

        /* Now we compute the conflict graph */
        for(uint32_t c1=0; c1<instp->num_characters; c1++)
                for(uint32_t c2=c1+1; c2<instp->num_characters; c2++) {
                        uint32_t states[2][2] = { {0, 0}, {0, 0} };
                        for(uint32_t s=0; s<instp->num_species; s++)
                                states[matrix_get_value(instp, s, c1)][matrix_get_value(instp, s, c2)] = 1;
                        if(states[0][0] + states[0][1] + states[1][0] + states[1][1] == 4)
                                igraph_add_edge(instp->conflict, instp->conflict_label[c1], instp->conflict_label[c2]);
                }
}





void copy_state(state_s* dst, const state_s* src) {
        assert(dst != NULL);
        if (dst->operation == NULL) dst->operation = new_operation();
        copy_operation(dst->operation, src->operation);
        if (dst->instance == NULL) dst->instance = new_instance();
        copy_instance(dst->instance, src->instance);
        dst->realized_char = src->realized_char;
        dst->tried_characters = g_slist_copy(src->tried_characters);
        dst->character_queue = g_slist_copy(src->character_queue);
}

void copy_operation(operation* dst, const operation* src) {
        assert(dst != NULL);
        dst->type = src->type;
        dst->removed_species_list = g_slist_copy(src->removed_species_list);
        dst->removed_characters_list = g_slist_copy(src->removed_characters_list);
        dst->removed_red_black_list = g_slist_copy(src->removed_red_black_list);
        dst->removed_conflict_list = g_slist_copy(src->removed_conflict_list);
}




/**
   To realize a character, first we have to find the id \c c of the vertex of
   the red-black graph encoding the input character.
   Then we find the connected component \c A of the red-black graph to which \c
   c belongs, and the set \c B of vertices adjacent to \c c.

   If \c is labeled black, we remove all edges from \c c to \c B and we add the
   edges from \c c to \c A. Finally, we label \c c as red.

   If \c c is already red, we check that A=B. In that case we remove all edges
   incident on \c c and we remove the vertex \c c (since it is free). On the
   other hand, if A is not equal to B, we return that the realization is
   impossible, setting \c error=1.
*/
pp_instance
realize_character(const pp_instance src, const uint32_t character, operation *op) {
        assert(op != NULL);
        pp_instance dst;
        copy_instance(&dst, &src);
        igraph_real_t c = dst.character_label[character];
        int ret = 0;

        igraph_vector_t conn_comp;
        igraph_vector_init(&conn_comp, 0);
        ret = igraph_subcomponent(dst.red_black, &conn_comp, c, IGRAPH_ALL);
        assert(ret == 0);
        igraph_vector_sort(&conn_comp);


        igraph_vector_t adjacent;
        igraph_vector_init(&adjacent, 0);
        ret = igraph_neighbors(dst.red_black, &adjacent, c, IGRAPH_ALL);
        assert(ret == 0);
        igraph_vector_t not_adjacent;
        igraph_vector_init(&not_adjacent, 0);
        igraph_vector_difference_sorted(&conn_comp, &adjacent, &not_adjacent);

        int color = VAN(dst.red_black, "color", c);
        assert(color != SPECIES);
        if (color == BLACK) {
                igraph_vector_t new_red;
                size_t l=igraph_vector_size(&not_adjacent);
                igraph_vector_init(&new_red, 2*l);
                for (size_t i=0; i<l; i++) {
                        VECTOR(new_red)[2*i] = c;
                        VECTOR(new_red)[2*i+1] = VECTOR(not_adjacent)[i];
                }
                igraph_add_edges(dst.red_black, &new_red, 0);
                igraph_vector_destroy(&new_red);

                igraph_vector_t to_delete;
                l=igraph_vector_size(&adjacent);
                igraph_vector_init(&to_delete, 2*l);
                igraph_es_t edges_to_delete;
                for (size_t i=0; i<l; i++) {
                        VECTOR(to_delete)[2*i] = c;
                        VECTOR(to_delete)[2*i+1] = VECTOR(adjacent)[i];
                }
                igraph_es_pairs(&edges_to_delete, &to_delete, IGRAPH_UNDIRECTED);
                igraph_delete_edges(dst.red_black, edges_to_delete);
                igraph_vector_destroy(&to_delete);
                igraph_vector_destroy(&not_adjacent);
                igraph_vector_destroy(&adjacent);
                igraph_vector_destroy(&conn_comp);
                igraph_es_destroy(&edges_to_delete);

                op->type = 1;
                SETVAN(dst.red_black, "color", c, RED);
                dst.root_state[character] = 1;
        }
        if (color == RED)
                if (igraph_vector_size(&adjacent) != igraph_vector_size(&conn_comp)) {
                        op->type = -1;
                } else {
                        igraph_delete_vertices(dst.red_black, igraph_vss_1(c));
                        dst.num_species--;
                        op->type = 2;
                        op->removed_characters_list = g_slist_append(op->removed_characters_list, GINT_TO_POINTER(character));
                        dst.root_state[character] = -1;
                }
        return dst;
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


pp_instance
read_instance_from_filename(const char *filename) {
        FILE* file;
        file = fopen(filename, "r");
        assert(file != NULL);
        assert(!feof(file));
        uint32_t num_species, num_characters;

        assert(fscanf(file, "%"SCNu32" %"SCNu32, &num_species, &num_characters) != EOF);
        pp_instance inst = { 0 };
        inst.num_species = num_species;
        inst.num_characters = num_characters;
        inst.matrix = GC_MALLOC(num_species * num_characters * sizeof(uint32_t));
        for(uint32_t s=0; s < num_species; s++)
                for(uint32_t c=0; c < num_characters; c++) {
                        uint32_t x;
                        assert(fscanf(file, "%"SCNu32, &x) != EOF);
                        matrix_set_value(&inst, s, c, x);
                }

        matrix2instance(&inst);
        char* str = NULL;
        str_instance(&inst, str);
        g_debug("%s", str);
        free(str);
        fclose(file);
        return inst;
}

#ifdef TEST_EVERYTHING
#include <check.h>

#endif

/*
  \brief Simplify the instance whenever possible.

*/

pp_instance
instance_cleanup(const pp_instance src, operation *op) {
        // remove isolated species and characters
        assert(op != NULL);
        int err;
        pp_instance dst;
        copy_instance(&dst, &src);
        igraph_vector_t clusters, sizes, isolated;
        err = igraph_vector_init(&clusters, 0);
        assert(err != 0);
        err = igraph_vector_init(&sizes, 1);
        assert(err != 0);
        err = igraph_vector_init(&isolated, 0);
        assert(err != 0);

        // Looking for null species
        for (uint32_t i=0; i < dst.num_species_orig; i++) {
                uint32_t id = dst.species_label[i];
                igraph_vector_t v;
                err = igraph_vector_init_seq(&v, id, id);
                err = igraph_degree(src.red_black, &sizes, igraph_vss_vector(&v), 0, IGRAPH_LOOPS);
                assert(err != 0);
                if (VECTOR(sizes)[0] == 1) {
                        op->removed_species_list = g_slist_append(op->removed_species_list, GINT_TO_POINTER(i));
                        op->type = 3;
                }
        }

        // Looking for null characters
        for (uint32_t i=0; i < dst.num_characters_orig; i++) {
                uint32_t id = dst.character_label[i];
                igraph_vector_t v;
                err = igraph_vector_init_seq(&v, id, id);
                err = igraph_degree(src.red_black, &sizes, igraph_vss_vector(&v), 0, IGRAPH_LOOPS);
                assert(err != 0);
                if (VECTOR(sizes)[0] == 1) {
                        op->removed_characters_list = g_slist_append(op->removed_characters_list, GINT_TO_POINTER(i));
                        op->type = 3;
                }
        }

/* TODO (if necessary) */
/* we remove duplicated characters */
/* we remove duplicated species */
        return dst;
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

#ifdef TEST_EVERYTHING
static void test_matrix_pp(pp_instance inst, const uint32_t num_species, const uint32_t num_characters,
                           const uint32_t data[inst.num_species][inst.num_characters],
                           const uint32_t conflict[inst.num_characters][inst.num_characters]) {
        ck_assert_int_eq(inst.num_species, num_species);
        ck_assert_int_eq(inst.num_characters, num_characters);
        ck_assert_int_eq(inst.num_species_orig, num_species);
        ck_assert_int_eq(inst.num_characters_orig, num_characters);
        for (uint32_t i=0; i<inst.num_species; i++)
                for (uint32_t j=0; j<inst.num_characters; j++)    {
                        ck_assert_int_eq(matrix_get_value(&inst, i, j), data[i][j]);
                        igraph_integer_t eid;
                        igraph_get_eid(inst.red_black, &eid, i, j+inst.num_species, 0, 0);
                        if (data[i][j] == 1)
                                ck_assert_int_ge(eid, 0);
                        else
                                ck_assert_int_lt(eid, 0);
                }
        for (uint32_t i=0; i<inst.num_species; i++)
                ck_assert_int_eq(inst.species_label[i], i);
        for (uint32_t i=0; i<inst.num_characters; i++)
                ck_assert_int_eq(inst.character_label[i], i+inst.num_species);
        if (conflict != NULL) {
                for (uint32_t c1=0; c1<inst.num_characters; c1++)
                        for (uint32_t c2=0; c2<inst.num_characters; c2++) {
                                igraph_integer_t eid;
                                igraph_get_eid(inst.conflict, &eid, inst.conflict_label[c1], inst.conflict_label[c2], 0, 0);
                                if (conflict[c1][c2] == 1)
                                        ck_assert_msg(eid >= 0, "Characters %d %d\n", c1, c2);
                                else
                                        ck_assert_msg(eid < 0, "Characters %d %d\n", c1, c2);
                        }
        }
}


START_TEST(test_read_instance_from_filename_1) {
        const uint32_t data[4][4] = {
                {0, 0, 1, 1},
                {0, 1, 0, 1},
                {1, 0, 1, 0},
                {1, 1, 0, 0}
        };
        const uint32_t conflict[4][4] = {
                {0, 1, 1, 0},
                {1, 0, 0, 1},
                {1, 0, 0, 1},
                {0, 1, 1, 0}
        };
        pp_instance inst = read_instance_from_filename("tests/input/read/1.txt");
        test_matrix_pp(inst, 4, 4, data, conflict);
}
END_TEST

START_TEST(test_read_instance_from_filename_2) {
        const uint32_t data[6][3] = {
                {0, 0, 1},
                {0, 1, 0},
                {0, 1, 1},
                {1, 0, 0},
                {1, 0, 1},
                {1, 1, 0}
        };
        const uint32_t conflict[3][3] = {
                {0, 1, 1},
                {1, 0, 1},
                {1, 1, 0}
        };
        pp_instance inst = read_instance_from_filename("tests/input/read/2.txt");
        test_matrix_pp(inst, 6, 3, data, conflict);
        //    igraph_write_graph_gml(inst.red_black, stdout, 0, 0);
}
END_TEST

START_TEST(test_read_instance_from_filename_3) {
        const uint32_t data[5][5] = {
                {0, 0, 0, 1, 0},
                {0, 1, 0, 0, 0},
                {1, 0, 1, 0, 0},
                {1, 1, 0, 0, 0},
                {0, 0, 0, 0, 0}
        };
        const uint32_t conflict[5][5] = {
                {0, 1, 0, 0, 0},
                {1, 0, 0, 0, 0},
                {0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0}
        };
        pp_instance inst = read_instance_from_filename("tests/input/read/3.txt");
        test_matrix_pp(inst, 5, 5, data, conflict);
        //igraph_write_graph_gml(inst.red_black, stdout, 0, 0);
}
END_TEST

#endif

/**
   \brief managing instances: \c new_instance \c init_instance \c
   destroy_instance \c free_instance

   \c destroy_instance does not free the instance, while \c free_instance does
*/
pp_instance *
new_instance(void) {
        pp_instance *instp = GC_MALLOC(sizeof(pp_instance));
        return instp;
}

#ifdef TEST_EVERYTHING
static void null_instance_test(pp_instance *instp) {
        ck_assert_msg(instp != NULL, "op has been freed\n");
        ck_assert_int_eq(instp->num_species, 0);
        ck_assert_int_eq(instp->num_characters, 0);
        ck_assert_int_eq(instp->num_species_orig, 0);
        ck_assert_int_eq(instp->num_characters_orig, 0);
        ck_assert_msg(instp->conflict == NULL, "instp->red_black has not been freed\n");
        ck_assert_msg(instp->red_black == NULL, "instp->red_black has not been freed\n");
        ck_assert_msg(instp->matrix == NULL, "instp->red_black has not been freed\n");
        ck_assert_msg(instp->species_label == NULL, "instp->red_black has not been freed\n");
        ck_assert_msg(instp->character_label == NULL, "instp->red_black has not been freed\n");
        ck_assert_msg(instp->conflict_label == NULL, "instp->red_black has not been freed\n");
        ck_assert_msg(instp->root_state == NULL, "instp->root_state has not been freed\n");
        ck_assert_msg(instp->species == NULL, "instp->species has not been freed\n");
        ck_assert_msg(instp->characters == NULL, "instp->characters has not been freed\n");
}

START_TEST(new_instance_1) {
        pp_instance * instp = new_instance();
        null_instance_test(instp);
}
END_TEST
#endif


void
copy_instance(pp_instance *dst, const pp_instance *src) {
        assert(dst != NULL);
        init_instance(dst, src->num_species, src->num_characters);

        dst->num_species = src->num_species;
        dst->num_characters = src->num_characters;
        dst->num_species_orig = src->num_species_orig;
        dst->num_characters_orig = src->num_characters_orig;
        igraph_copy(dst->red_black, src->red_black);
        igraph_copy(dst->conflict, src->conflict);
        dst->matrix = src->matrix;
        memcpy(dst->species_label, src->species_label, src->num_species_orig * sizeof(uint32_t));
        memcpy(dst->character_label, src->character_label, src->num_characters_orig * sizeof(uint32_t));
        memcpy(dst->conflict_label, src->conflict_label, src->num_characters_orig * sizeof(uint32_t));
        memcpy(dst->root_state, src->root_state, src->num_characters_orig * sizeof(uint32_t));
        dst->species = g_slist_copy(src->species);
        dst->characters = g_slist_copy(src->characters);
}

void str_instance(const pp_instance* instp, char* str) {
        assert(0 <= asprintf(&str,
                             "Instance: {\n"
                             "  num_species: %d\n"
                             "  num_characters: %d\n"
                             "  num_species_orig: %d\n"
                             "  num_characters_orig: %d\n"
                             "  red_black: %p\n"
                             "  conflict: %p\n"
                             "  matrix: %p\n"
                             "  species_label: %p\n"
                             "  character_label: %p\n"
                             "  conflict_label: %p\n"
                             "  root_state: %p\n"
                             "  species: %p\n"
                             "  characters: %p\n"
                             "}",
                             instp->num_species,
                             instp->num_characters,
                             instp->num_species_orig,
                             instp->num_characters_orig,
                             (void *) instp->red_black,
                             (void *) instp->conflict,
                             (void *) instp->matrix,
                             (void *) instp->species_label,
                             (void *) instp->character_label,
                             (void *) instp->conflict_label,
                             (void *) instp->root_state,
                             (void *) instp->species,
                             (void *) instp->characters
                       ));
}

void
destroy_instance(pp_instance *instp) {
        /* if (instp->conflict != NULL) */
        /*     igraph_destroy(instp->conflict); */
        /* if (instp->red_black != NULL) */
        /*     igraph_destroy(instp->red_black); */
        /* free(instp->matrix); */
        /* free(instp->species_label); */
        /* free(instp->character_label); */
        /* free(instp->conflict_label); */
        /* free(instp->root_state); */
        /* g_slist_free(instp->species); */
        /* g_slist_free(instp->characters); */
}

#ifdef TEST_EVERYTHING
/* START_TEST(destroy_instance_1) { */
/*     pp_instance *instp = new_instance(); */
/*     destroy_instance(instp); */
/*     null_instance_test(instp); */
/* } */
/* END_TEST */
#endif


void
free_instance(pp_instance *instp) {
        /* destroy_instance(instp); */
        /* free(instp); */
}



/**
   \brief managing operations: \c new_operation \c init_operation \c
   destroy_operation
*/
operation *
new_operation(void) {
        operation *op = GC_MALLOC(sizeof(operation));
        init_operation(op);
        return op;
}

#ifdef TEST_EVERYTHING
START_TEST(new_operation_1) {
        operation *op = new_operation();
        ck_assert_msg(op != NULL, "op has been freed\n");
        ck_assert_msg(op->removed_species_list == NULL, "removed_species_list has not been freed\n");
        ck_assert_msg(op->removed_characters_list == NULL, "removed_characters_list has not been freed\n");
        ck_assert_msg(op->removed_red_black_list == NULL, "removed_red_black_list has not been freed\n");
        ck_assert_msg(op->removed_conflict_list == NULL, "removed_conflict_list has not been freed\n");
        ck_assert_int_eq(op->removed_characters_list, 0);
}
END_TEST
#endif



void
init_operation(operation *op) {
        assert(op != NULL);
        operation temp = {
                .type = 0,
                .removed_species_list = NULL,
                .removed_characters_list = NULL,
                .removed_red_black_list = NULL,
                .removed_conflict_list = NULL
        };
        *op = temp;
}


void
destroy_operation(operation *op) {
        /* if (op->removed_species_list != NULL) */
        /*     g_slist_free(op->removed_species_list); */
        /* if (op->removed_characters_list != NULL) */
        /*     g_slist_free(op->removed_characters_list); */
        /* if (op->removed_red_black_list != NULL) */
        /*     g_slist_free(op->removed_red_black_list); */
        /* if (op->removed_conflict_list != NULL) */
        /*     g_slist_free(op->removed_conflict_list); */
        /* op->removed_characters_list = 0; */
}


#ifdef TEST_EVERYTHING
/* START_TEST(destroy_operation_1) { */
/*     operation *op = new_operation(); */
/*     destroy_operation(op); */
/*     ck_assert_msg(op != NULL, "op has been freed\n"); */
/*     ck_assert_msg(op->removed_species_list == NULL, "removed_species_list has not been freed\n"); */
/*     ck_assert_msg(op->removed_characters_list == NULL, "removed_characters_list has not been freed\n"); */
/*     ck_assert_msg(op->removed_red_black_list == NULL, "removed_red_black_list has not been freed\n"); */
/*     ck_assert_msg(op->removed_conflict_list == NULL, "removed_conflict_list has not been freed\n"); */
/*     ck_assert_int_eq(op->removed_characters_list, 0); */
/* } */
/* END_TEST */
#endif

void
free_operation(operation *op) {
        /* destroy_operation(op); */
        /* free(op); */
}

state_s*
new_state(void) {
        state_s *stp = GC_MALLOC(sizeof(state_s));
        init_state(stp);
        return stp;
}

void
init_state(state_s *stp) {
        assert(stp != NULL);
        state_s temp = {
                .operation = new_operation(),
                .instance = new_instance(),
                .realized_char = 0,
                .tried_characters = NULL,
                .character_queue = NULL,
        };
        *stp = temp;
}

void
destroy_state(state_s *stp) {
        /* free_instance(stp->instance); */
        /* free_operation(stp->operation); */
        /* if (stp->tried_characters != NULL) */
        /*     g_slist_free(stp->tried_characters); */
}

void
free_state(state_s *stp) {
        /* destroy_state(stp); */
        /* free(stp); */
}

uint32_t check_state(const state_s* stp) {
        uint32_t err = 0;
        pp_instance* instp = stp->instance;
        if (instp->num_species != g_slist_length(instp->species)) {
                err += 1;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %d)", instp->num_species, g_slist_length(instp->species));
        }
        if (instp->num_characters != g_slist_length(instp->characters)) {
                err += 2;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %d)", instp->num_characters, g_slist_length(instp->characters));
        }
        if (instp->num_species != sizeof(instp->species_label)/sizeof(instp->species_label[0])) {
                err += 4;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %lu)", instp->num_species , sizeof(instp->species_label)/sizeof(instp->species_label[0]));
        }
        if (instp->num_species != sizeof(instp->root_state)/sizeof(instp->root_state[0])) {
                err += 8;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %lu)", instp->num_species , sizeof(instp->root_state)/sizeof(instp->root_state[0]));
        }
        if (instp->num_characters != sizeof(instp->character_label)/sizeof(instp->character_label[0])) {
                err += 16;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %lu)", instp->num_characters, sizeof(instp->character_label)/sizeof(instp->character_label[0]));
        }
        if (instp->num_characters != sizeof(instp->conflict_label)/sizeof(instp->conflict_label[0])) {
                err += 32;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %lu)", instp->num_characters, sizeof(instp->conflict_label)/sizeof(instp->conflict_label[0]));
        }
        return err;
}

static GSList* json_array2gslist(json_t* array) {
        GSList* list = NULL;
        size_t index;
        json_t *value;
        json_array_foreach(array, index, value)
                list = g_slist_prepend(list, GINT_TO_POINTER(json_integer_value(value)));
        return g_slist_reverse(list);
}

static uint32_t* json_array2array(json_t* array) {
        size_t index;
        json_t *value;
        uint32_t* new = GC_MALLOC(json_array_size(array) * sizeof(uint32_t));
        assert(new != NULL);
        json_array_foreach(array, index, value) {
                new[index] = json_integer_value(value);
        }
        return new;
}

static uint32_t json_get_integer(const json_t* root, const char* field) {
        json_t* obj = json_object_get(root, field);
        assert(obj != NULL && "Missing JSON field\n");
        assert(json_is_integer(obj) && "value must be an integer\n");
        return json_integer_value(obj);
}

static char* json_get_string(json_t* root, char* field) {
        json_t* obj = json_object_get(root, field);
        assert(obj != NULL && "Missing JSON field\n");
        assert(json_is_string(obj) && "value must be an integer\n");
        char* dst = GC_MALLOC(sizeof(char) * 300); // json_string_length
        dst = strdup(json_string_value(obj));
        return dst;
}

static uint32_t* json_get_array(json_t* root, char* field) {
        json_t* obj = json_object_get(root, field);
        assert(obj != NULL && "Missing JSON field\n");
        assert(json_is_array(obj) && "field must be an array\n");
        return json_array2array(obj);
}

static GSList* json_get_list(json_t* root, char* field, bool optional) {
        json_t* obj = json_object_get(root, field);
        if (!optional) {
                assert(obj != NULL && "Missing JSON field\n");
                assert(json_is_array(obj) && "field must be an array\n");
                return json_array2gslist(obj);
        } else
                return NULL;
}


state_s*
read_state(const char* filename) {
        json_set_alloc_funcs(GC_malloc, GC_free);
        state_s* stp = new_state();
        operation* op = new_operation();
        pp_instance* instp = new_instance();
        stp->instance = instp;
        stp->operation = op;

        json_t* data = json_load_file(filename, JSON_DISABLE_EOF_CHECK , NULL);
        assert(data != NULL && "Could not parse JSON file\n");

        stp->realized_char = json_get_integer(data, "realized_char");
        stp->tried_characters = json_get_list(data, "tried_characters", true);
        stp->character_queue = json_get_list(data, "character_queue", true);

        json_t* instpj = json_object_get(data, "instance");
        if (instpj != NULL) {
                stp->instance->num_species = json_get_integer(instpj, "num_species");
                stp->instance->num_characters = json_get_integer(instpj, "num_characters");
                stp->instance->num_species_orig = json_get_integer(instpj, "num_species_orig");
                stp->instance->num_characters_orig = json_get_integer(instpj, "num_characters_orig");
                stp->instance->species_label = json_get_array(instpj, "species_label");
                stp->instance->character_label = json_get_array(instpj, "character_label");
                stp->instance->conflict_label = json_get_array(instpj, "conflict_label");
                stp->instance->root_state = json_get_array(instpj, "root_state");
                stp->instance->species = json_get_list(instpj, "species", false);
                stp->instance->characters = json_get_list(instpj, "characters", false);

                // Graphs
                FILE* fp;
                stp->instance->red_black = GC_MALLOC(sizeof(igraph_t));
                fp = fopen(json_get_string(instpj, "red_black_file"), "r");
                assert(fp != NULL && "Cannot open file\n");
                igraph_read_graph_graphml(stp->instance->red_black, fp, 0);
                fclose(fp);

                stp->instance->conflict = GC_MALLOC(sizeof(igraph_t));
                fp = fopen(json_get_string(instpj, "conflict_file"), "r");
                assert(fp != NULL && "Cannot open file\n");
                igraph_read_graph_graphml(stp->instance->conflict, fp, 0);
                fclose(fp);
        }

        /* operation */
        json_t* opj = json_object_get(data, "operation");
        if (opj != NULL) {
                stp->operation->type = json_get_integer(opj, "type");
                stp->operation->removed_species_list = json_get_list(opj, "removed_species_list", false);
                stp->operation->removed_characters_list = json_get_list(opj, "removed_characters_list", false);
                stp->operation->removed_red_black_list = json_get_list(opj, "removed_red_black_list", false);
                stp->operation->removed_conflict_list = json_get_list(opj, "removed_conflict_list", false);
        }
        return stp;
}

static json_t* gslist2json_array(GSList* list) {
        json_t* array = json_array();
        for(;list != NULL; list = g_slist_next(list))
                json_array_append(array, json_integer(GPOINTER_TO_INT(list->data)));
        return array;
}

static json_t* array2json_array(uint32_t* p, size_t size) {
        json_t* array = json_array();
        for(size_t i=0; i<size; i++)
                json_array_append(array, json_integer(p[i]));
        return array;
}

/**
   Since we use JSON as exchange format, we first have to build the JSON object
   representing the state.

   Since the igraph library provides the  \c igraph_write_graph_graphml
   and \c igraph_read_graph_graphml functions that read/write a graphml file (an
   XML that is quite readable), instead of encoding the graphs in JSON, we
   include only the filenames and we export the graphs in GraphML
*/
void
write_state(const char* filename, state_s* stp) {
        json_set_alloc_funcs(GC_malloc, GC_free);
        json_t* data = json_object();
        assert(!json_object_set(data, "realized_char", json_integer(stp->realized_char)));
        assert(!json_object_set(data, "tried_characters", gslist2json_array(stp->tried_characters)));
        assert(!json_object_set(data, "character_queue", gslist2json_array(stp->character_queue)));

        json_t* instp = json_object();
        json_t* op = json_object();
        assert(!json_object_set(data, "instance", instp));

        /* pp_instance */
        assert(!json_object_set(instp, "num_species", json_integer(stp->instance->num_species)));
        assert(!json_object_set(instp, "num_characters", json_integer(stp->instance->num_characters)));
        assert(!json_object_set(instp, "num_species_orig", json_integer(stp->instance->num_species_orig)));
        assert(!json_object_set(instp, "num_characters_orig", json_integer(stp->instance->num_characters_orig)));

        char* g_filename = NULL;
        FILE* fp = NULL;
        if (stp->instance->red_black != NULL) {
                asprintf(&g_filename, "%s-redblack.graphml", filename);
                fp = fopen(g_filename, "w");
                assert(fp != NULL);
                assert(!igraph_write_graph_graphml(stp->instance->red_black, fp));
                fclose(fp);
                assert(!json_object_set(instp, "red_black_file", json_string(g_filename)));
        }

        if (stp->instance->conflict != NULL) {
                asprintf(&g_filename, "%s-conflict.graphml", filename);
                fp = fopen(g_filename, "w");
                assert(!igraph_write_graph_graphml(stp->instance->conflict, fp));
                fclose(fp);
                assert(!json_object_set(instp, "conflict_file", json_string(g_filename)));
        }
        free(g_filename);

        if (stp->instance->matrix != NULL)
                assert(!json_object_set(instp, "matrix", array2json_array(stp->instance->matrix, stp->instance->num_species * stp->instance->num_characters)));
        assert(!json_object_set(instp, "species_label", array2json_array(stp->instance->species_label, stp->instance->num_species)));
        assert(!json_object_set(instp, "character_label", array2json_array(stp->instance->character_label, stp->instance->num_characters)));
        assert(!json_object_set(instp, "conflict_label", array2json_array(stp->instance->conflict_label, stp->instance->num_characters)));
        assert(!json_object_set(instp, "root_state", array2json_array(stp->instance->root_state, stp->instance->num_species)));
        assert(!json_object_set(instp, "species", gslist2json_array(stp->instance->species)));
        assert(!json_object_set(instp, "characters", gslist2json_array(stp->instance->characters)));

        /* operation */
        if (stp->operation != NULL) {
                assert(!json_object_set(op, "type", json_integer(stp->operation->type)));
                assert(!json_object_set(op, "removed_species_list", gslist2json_array(stp->operation->removed_species_list)));
                assert(!json_object_set(op, "removed_characters_list", gslist2json_array(stp->operation->removed_characters_list)));
                assert(!json_object_set(op, "removed_red_black_list", gslist2json_array(stp->operation->removed_red_black_list)));
                assert(!json_object_set(op, "removed_conflict_list", gslist2json_array(stp->operation->removed_conflict_list)));
                assert(!json_object_set(data, "operation", op));
        }

        assert(!json_dump_file(data, filename, JSON_INDENT(4) | JSON_SORT_KEYS) && "Cannot write JSON file\n");
}
#ifdef TEST_EVERYTHING
/* START_TEST(write_json_1) { */
/*     state_s *stp = new_state(); */
/*     stp->realized_char = 1; */
/*     write_state("tests/api/1.json", stp); */

/*     state_s *stp2 = read_state("tests/api/1.json"); */
/*     ck_assert_int_eq(stp->realized_char, stp2->realized_char); */
/* } */
/* END_TEST */
#endif


void first_state(state_s* stp, pp_instance *instp) {
        if(stp->instance != instp) {
                stp->instance = new_instance();
                copy_instance(stp->instance, instp);
        }
        stp->operation = new_operation();
        stp->realized_char = 0;
        stp->tried_characters = NULL;
        stp->character_queue = NULL;

        stp->operation->type = 0;
        stp->operation->removed_species_list = NULL;
        stp->operation->removed_characters_list = NULL;
        stp->operation->removed_red_black_list = NULL;
        stp->operation->removed_conflict_list = NULL;
}


#ifdef TEST_EVERYTHING
/* START_TEST(write_json_2) { */
/*     state_s *stp = new_state(); */
/*     stp->realized_char = 1; */
/*     stp->tried_characters = g_slist_append(stp->tried_characters, GINT_TO_POINTER(91)); */
/*     stp->tried_characters = g_slist_append(stp->tried_characters, GINT_TO_POINTER(92)); */
/*     stp->tried_characters = g_slist_append(stp->tried_characters, GINT_TO_POINTER(93)); */
/*     stp->tried_characters = g_slist_append(stp->tried_characters, GINT_TO_POINTER(95)); */
/*     stp->tried_characters = g_slist_append(stp->tried_characters, GINT_TO_POINTER(96)); */
/*     stp->tried_characters = g_slist_append(stp->tried_characters, GINT_TO_POINTER(97)); */
/*     stp->tried_characters = g_slist_append(stp->tried_characters, GINT_TO_POINTER(98)); */
/*     write_state("tests/api/2.json", stp); */

/*     state_s *stp2 = read_state("tests/api/2.json"); */
/*     ck_assert_int_eq(stp->realized_char, stp2->realized_char); */
/*     for (size_t i=0; i<7; i++) */
/*         ck_assert_int_eq(GPOINTER_TO_INT(g_slist_nth_data(stp->tried_characters, i)), */
/*             GPOINTER_TO_INT(g_slist_nth_data(stp2->tried_characters, i))); */
/* } */
/* END_TEST */

START_TEST(realize_3_0) {
        state_s *stp = read_state("tests/api/3.json");
        operation* op = new_operation();
        pp_instance inst2 = realize_character(*stp->instance, 0, op);
        state_s* stp2 = new_state();
        copy_state(stp2, stp);
        stp2->instance = &inst2;
        write_state("tests/api/3-0.json", stp2);

        ck_assert_int_eq(stp2->realized_char, 0);
}
END_TEST

START_TEST(write_json_3) {
        state_s *stp = new_state();
        stp->instance = new_instance();
        *stp->instance = read_instance_from_filename("tests/input/read/3.txt");
        write_state("tests/api/3.json", stp);

        state_s *stp2 = read_state("tests/api/3.json");
        ck_assert_int_eq(stp->realized_char, stp2->realized_char);
}
END_TEST

static int g_slist_cmp(GSList* l1, GSList* l2) {
        if (l1 == NULL && l2 == NULL) return 0;
        if (l1 == NULL) return 1;
        if (l2 == NULL) return -1;
        GSList* x1 = g_slist_next(l1);
        GSList* x2 = g_slist_next(l2);
        for (;x1 != NULL && x2 != NULL; x1 = g_slist_next(x1), x2 = g_slist_next(x2)) {
                int32_t d = GPOINTER_TO_INT(x2->data) - GPOINTER_TO_INT(x1->data);
                if (d != 0) return d;
        }
        if (x1 == NULL && x2 == NULL) return 0;
        return (x1 == NULL) ? 1 : -1;
}
static uint32_t instance_cmp(pp_instance *instp1, pp_instance *instp2) {
        uint32_t result = 0;
        if (instp1->num_characters != instp2->num_characters) result += 1;
        if (instp1->num_species != instp2->num_species) result += 2;
        if (instp1->species_label == NULL || instp2->species_label == NULL ||
            memcmp(instp1->species_label, instp2->species_label, sizeof(*(instp1->species_label)))) result += 4;
        if (instp1->character_label == NULL || instp2->character_label == NULL ||
            memcmp(instp1->character_label, instp2->character_label, sizeof(*(instp1->character_label)))) result += 8;
        if (instp1->conflict_label == NULL || instp2->conflict_label == NULL ||
            memcmp(instp1->conflict_label, instp2->conflict_label, sizeof(*(instp1->conflict_label)))) result += 16;
        if (instp1->root_state == NULL || instp2->root_state == NULL ||
            memcmp(instp1->root_state, instp2->root_state, sizeof(*(instp1->root_state)))) result += 32;
        if (g_slist_cmp(instp1->species, instp2->species)) result += 64;
        if (g_slist_cmp(instp1->characters, instp2->characters)) result += 128;
        return result;
}
START_TEST(copy_instance_1) {
        pp_instance inst = read_instance_from_filename("tests/input/read/1.txt");
        pp_instance inst2 = { 0 };
        copy_instance(&inst2, &inst);
        ck_assert_int_eq(instance_cmp(&inst, &inst2),0);
}
END_TEST
START_TEST(copy_instance_2) {
        pp_instance inst = read_instance_from_filename("tests/input/read/2.txt");
        pp_instance inst2 = { 0 };
        copy_instance(&inst2, &inst);
        ck_assert_int_eq(instance_cmp(&inst, &inst2),0);
}
END_TEST

static Suite * perfect_phylogeny_suite(void) {
        Suite *s;
        TCase *tc_core;

        s = suite_create("perfect_phylogeny.c");

/* Core test case */
        tc_core = tcase_create("Core");

/* tcase_add_test(tc_core, init_instance_1); */
        tcase_add_test(tc_core, test_read_instance_from_filename_1);
        tcase_add_test(tc_core, test_read_instance_from_filename_2);
        tcase_add_test(tc_core, new_instance_1);
/* tcase_add_test(tc_core, destroy_instance_1); */
        tcase_add_test(tc_core, copy_instance_1);
        tcase_add_test(tc_core, copy_instance_2);
        tcase_add_test(tc_core, new_operation_1);
/* tcase_add_test(tc_core, destroy_operation_1); */

        tcase_add_test(tc_core, test_read_instance_from_filename_3);
/* tcase_add_test(tc_core, write_json_1); */
/* tcase_add_test(tc_core, write_json_2); */
        tcase_add_test(tc_core, write_json_3);
        tcase_add_test(tc_core, realize_3_0);

        suite_add_tcase(s, tc_core);

        return s;
}

int main(int argc, char **argv) {
        igraph_i_set_attribute_table(&igraph_cattribute_table);
        if(argc < 2) {
                Suite *s;
                SRunner *sr;
                s = perfect_phylogeny_suite();
                sr = srunner_create(s);
                srunner_run_all(sr, CK_NORMAL);
                int number_failed = srunner_ntests_failed(sr);
                srunner_free(sr);
                return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        json_t* data = json_load_file(argv[1], JSON_DISABLE_EOF_CHECK , NULL);
        assert(data != NULL && "Could not parse JSON file\n");
        unsigned int test_type = json_integer_value(json_object_get(data,"test"));
        if (test_type == 1) {
                const char *input_json_filename = json_string_value(json_object_get(data,"input"));
                json_t* listc = json_object_get(data, "characters");
                size_t index;
                json_t *value;
                state_s *stp = read_state(input_json_filename);
                json_array_foreach(listc, index, value) {
                        operation* op = new_operation();
                        pp_instance inst2 = realize_character(*stp->instance, json_integer_value(value), op);
                        stp->instance = &inst2;
                }
                write_state(json_string_value(json_object_get(data,"output")), stp);
        }
        return EXIT_SUCCESS;
}
#endif
