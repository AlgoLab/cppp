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

void
copy_instance(pp_instance *dst, const pp_instance *src) {
    if (dst->red_black == NULL) dst->red_black = g_malloc0(sizeof(igraph_t));
    if (dst->conflict == NULL)  dst->conflict  = g_malloc0(sizeof(igraph_t));

    dst->num_species = src->num_species;
    dst->num_characters = src->num_characters;
    dst->num_species_orig = src->num_species_orig;
    dst->num_characters_orig = src->num_characters_orig;

    igraph_copy(dst->red_black, src->red_black);
    igraph_copy(dst->conflict, src->conflict);
    dst->matrix = src->matrix;
    dst->species_label = g_malloc(src->num_species_orig * sizeof(uint32_t));
    memcpy(dst->species_label, src->species_label, src->num_species_orig * sizeof(uint32_t));
    dst->character_label = g_malloc(src->num_characters_orig * sizeof(uint32_t));
    memcpy(dst->character_label, src->character_label, src->num_characters_orig * sizeof(uint32_t));
    dst->conflict_label = g_malloc(src->num_characters_orig * sizeof(uint32_t));
    memcpy(dst->conflict_label, src->conflict_label, src->num_characters_orig * sizeof(uint32_t));
    dst->root_state = g_malloc(src->num_characters_orig * sizeof(uint8_t));
    memcpy(dst->root_state, src->root_state, src->num_characters_orig * sizeof(uint8_t));
    dst->species = g_slist_copy(src->species);
    dst->characters = g_slist_copy(src->characters);
}

#ifdef TEST_EVERYTHING
static int g_slist_cmp(GSList* l1, GSList* l2) {
    while (uint32_t x1 = g_slist_next(l1) && uint32_t x1 = g_slist_next(l2)) {
        if (x1 < x2) return 1;
        if (x2 < x1) return -1;
    }
    if (x1 == NULL && x2 == NULL) return 0;
    return (x1 == NULL) ? 1 : -1;
}
static uint32_t instance_cmp(pp_instance *instp1, pp_instance *instp2) {
    uint32_t result = 0;
    result += (instp1->num_characters != instp2->num_characters) ? 0x0001 : 0;
    result += (instp1->num_species != instp2->num_species)       ? 0x0002 : 0;
    result += instp1->species_label == NULL || instp2->species_label == NULL ||
        (memcmp(instp1->species_label, instp2->species_label, sizeof(*(instp1->species_label))) != 0) ? 0x0004 : 0;
    result += instp1->character_label == NULL || instp2->character_label == NULL ||
        (memcmp(instp1->character_label, instp2->character_label, sizeof(*(instp1->character_label))) != 0) ? 0x0008 : 0;
    result += instp1->conflict_label == NULL || instp2->conflict_label == NULL ||
        (memcmp(instp1->conflict_label, instp2->conflict_label, sizeof(*(instp1->conflict_label))) != 0) ? 0x0010 : 0;
    result += instp1->root_state == NULL || instp2->root_state == NULL ||
        (memcmp(instp1->root_state, instp2->root_state, sizeof(*(instp1->root_state))) != 0) ? 0x0011 : 0;
    result += instp1->species == NULL || instp2->species == NULL ||
        (g_slist_cmp(instp1->species, instp2->species) != 0) ? 0x0010 : 0;
    result += instp1->characters == NULL || instp2->characters == NULL ||
        (g_slist_cmp(instp1->characters, instp2->characters) != 0) ? 0x0010 : 0;
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
#endif

/**
   To realize a character, first we have to find the id \c c of the vertex of
   the red-black graph encoding the input character.
   Then we find the connected component \c A of the
   red-black graph to which \c c belongs, and the set \c B of vertices adjacent
   to \c c.

   If \c is labeled black,
   we remove all edges from \c c to \c B and we add the edges from \c c to
   \c A. Finally, we label \c c as red.

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

    igraph_vector_t conn_comp;
    igraph_vector_init(&conn_comp, dst.num_species+dst.num_characters);
    assert(igraph_subcomponent(dst.red_black, &conn_comp, c, IGRAPH_ALL) == 0);
    igraph_vector_sort(&conn_comp);
    igraph_vector_ptr_t res;
    igraph_vector_ptr_init(&res, 1);
    igraph_neighborhood(dst.red_black, &res, igraph_vss_1(c), 1, IGRAPH_ALL);
    igraph_vector_t adjacent;
    igraph_vector_copy(&adjacent, VECTOR(res)[0]);
    igraph_vector_sort(&adjacent);
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
            igraph_es_pairs(&edges_to_delete, &to_delete, IGRAPH_UNDIRECTED);
            igraph_delete_edges(dst.red_black, edges_to_delete);
        }
        igraph_vector_destroy(&to_delete);
        igraph_vector_destroy(&not_adjacent);
        igraph_vector_destroy(&adjacent);
        igraph_vector_destroy(&conn_comp);
        igraph_es_destroy(&edges_to_delete);

        op->type = 1;
        SETVAN(dst.red_black, "color", c, RED);
    }
    if (color == RED)
        if (igraph_vector_size(&adjacent) == igraph_vector_size(&conn_comp)) {
            op->type = 0;
        } else {
            igraph_delete_vertices(dst.red_black, igraph_vss_1(c));
            dst.num_species--;
            op->type = 2;
            op->removed_characters_list = g_slist_append(op->removed_characters_list, GINT_TO_POINTER(character));
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

/**
   \param instance

   Updates an instance by computing the red-black and the conflict graphs
   associated to a given matrix.

   In a red-black graph, the first \c instp->num_species ids correspond to species,
   while the ids larger or equal to instp->num_species correspond to characters.
   Notice that the label id must be conserved when modifying the graph (i.e.
   realizing a character).

   color attribute is \c SPECIES if the vertex is a species, otherwise it is \c BLACK
   or \c RED (at the beginning, there can only be \c BLACK edges).

*/
static void
matrix2graphs(pp_instance *instp) {
    assert(instp->matrix != NULL);

    instp->species_label = g_malloc(instp->num_species * sizeof(uint32_t));
    instp->character_label = g_malloc(instp->num_characters * sizeof(uint32_t));
    instp->conflict_label = g_malloc(instp->num_species * sizeof(uint32_t));

/* We start with the red-black graph */
    instp->red_black = g_malloc(sizeof(igraph_t));
    igraph_empty_attrs(instp->red_black, instp->num_species+instp->num_characters, IGRAPH_UNDIRECTED, 0);

    for(uint32_t s=0; s<instp->num_species; s++) {
        SETVAN(instp->red_black, "id", s, s);
        instp->species_label[s] = s;
        SETVAN(instp->red_black, "color", s, SPECIES);
    }
    for(uint32_t c=0; c<instp->num_characters; c++) {
        SETVAN(instp->red_black, "id", c+instp->num_species, c);
        instp->character_label[c] = c+instp->num_species;
        instp->conflict_label[c] = c;
        SETVAN(instp->red_black, "color", c+instp->num_species, BLACK);
    }
    for (uint32_t s=0; s<instp->num_species; s++)
        for (uint32_t c=0; c<instp->num_characters; c++)
            if (matrix_get_value(instp, s, c) == 1)
                igraph_add_edge(instp->red_black, s, c+instp->num_species);

    /* Now we compute the conflict graph */
    instp->conflict = g_malloc(sizeof(igraph_t));
    igraph_empty(instp->conflict, instp->num_characters, IGRAPH_UNDIRECTED);
    // TODO
    for(uint32_t c1=0; c1<instp->num_characters; c1++)
        for(uint32_t c2=c1+1; c2<instp->num_characters; c2++) {
            uint8_t states[2][2] = { {0, 0}, {0, 0} };
            for(uint32_t s=0; s<instp->num_species; s++)
                states[matrix_get_value(instp, s, c1)][matrix_get_value(instp, s, c2)] = 1;
            if(states[0][0] + states[0][1] + states[1][0] + states[1][1] == 4)
                igraph_add_edge(instp->conflict, instp->conflict_label[c1], instp->conflict_label[c2]);
        }
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
    inst.num_species_orig = num_species;
    inst.num_characters_orig = num_characters;
    inst.matrix = g_malloc(num_species * num_characters * sizeof(uint8_t));
    inst.root_state = g_malloc0(num_species * sizeof(uint8_t));
    for(uint32_t s=0; s < num_species; s++)
        for(uint32_t c=0; c < num_characters; c++) {
            uint8_t x;
            assert(fscanf(file, "%"SCNu8, &x) != EOF);
            matrix_set_value(&inst, s, c, x);
        }

    matrix2graphs(&inst);
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
static void test_matrix_pp(pp_instance inst, const uint8_t num_species, const uint8_t num_characters,
    const uint8_t data[inst.num_species][inst.num_characters],
    const uint8_t conflict[inst.num_characters][inst.num_characters]) {
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
    const uint8_t data[4][4] = {
        {0, 0, 1, 1},
        {0, 1, 0, 1},
        {1, 0, 1, 0},
        {1, 1, 0, 0}
    };
    const uint8_t conflict[4][4] = {
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
    const uint8_t data[6][3] = {
        {0, 0, 1},
        {0, 1, 0},
        {0, 1, 1},
        {1, 0, 0},
        {1, 0, 1},
        {1, 1, 0}
    };
    const uint8_t conflict[3][3] = {
        {0, 1, 1},
        {1, 0, 1},
        {1, 1, 0}
    };
    pp_instance inst = read_instance_from_filename("tests/input/read/2.txt");
    test_matrix_pp(inst, 6, 3, data, conflict);
    //    igraph_write_graph_gml(inst.red_black, stdout, 0, 0);
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
    pp_instance *instp = g_malloc0(sizeof(pp_instance));
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
    ck_assert_msg(instp->character == NULL, "instp->characters has not been freed\n");
}

START_TEST(new_instance_1) {
    pp_instance * instp = new_instance();
    null_instance_test(instp);
}
END_TEST
#endif

/* void */
/* init_instance(pp_instance * instp) { */
/*     assert(instp != NULL); */
/*     instp->conflict = g_malloc(sizeof(igraph_t)); */
/*     igraph_empty(instp->conflict, instp->num_characters, IGRAPH_UNDIRECTED); */
/*     instp->red_black = = g_malloc(sizeof(igraph_t)); */
/*     igraph_empty_attrs(instp->red_black, instp->num_species + instp->num_characters, IGRAPH_UNDIRECTED); */
/*     instp->matrix =  NULL; */
/*     instp->species_label = g_malloc(instp->num_species * sizeof(uint32_t)); */
/*     instp->character_label = g_malloc(instp->num_characters * sizeof(uint32_t)); */
/*     instp->conflict_label = g_malloc(instp->num_species * sizeof(uint32_t)); */
/* } */


/* #ifdef TEST_EVERYTHING */
/* /\* START_TEST(init_instance_1) { *\/ */
/* /\*     pp_instance inst = { 0 }; *\/ */
/* /\*     null_instance_test(&inst); *\/ */
/* /\* } *\/ */
/* /\* END_TEST *\/ */
/* #endif */

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
    if (instp->conflict != NULL)
        igraph_destroy(instp->conflict);
    if (instp->red_black != NULL)
        igraph_destroy(instp->red_black);
    free(instp->matrix);
    free(instp->species_label);
    free(instp->character_label);
    free(instp->conflict_label);
    free(instp->root_state);
    g_slist_free(species);
    g_slist_free(characters);
}

#ifdef TEST_EVERYTHING
START_TEST(destroy_instance_1) {
    pp_instance *instp = new_instance();
    destroy_instance(instp);
    null_instance_test(instp);
}
END_TEST
#endif


void
free_instance(pp_instance *instp) {
    destroy_instance(instp);
    free(instp);
}

/**
   \brief managing operations: \c new_operation \c init_operation \c
   destroy_operation
*/
operation *
new_operation(void) {
    operation *op = g_malloc0(sizeof(operation));
    init_operation(op);
    return op;
}

#ifdef TEST_EVERYTHING
START_TEST(new_operation_1) {
    operation *op = new_operation();
    ck_assert_msg(op != NULL, "op has been freed\n");
    ck_assert_msg(op->removed_species_list == NULL, "removed_species_list has not been freed\n");
    ck_assert_msg(op->removed_characters_list == NULL, "removed_characters_list has not been freed\n");
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
        .removed_characters_list = NULL
    };
    *op = temp;
}


void
destroy_operation(operation *op) {
    if (op->removed_species_list != NULL)
        g_slist_free(op->removed_species_list);
    if (op->removed_characters_list != NULL)
        g_slist_free(op->removed_characters_list);
    op->removed_characters_list = 0;
}


#ifdef TEST_EVERYTHING
START_TEST(destroy_operation_1) {
    operation *op = new_operation();
    destroy_operation(op);
    ck_assert_msg(op != NULL, "op has been freed\n");
    ck_assert_msg(op->removed_species_list == NULL, "removed_species_list has not been freed\n");
    ck_assert_msg(op->removed_characters_list == NULL, "removed_characters_list has not been freed\n");
    ck_assert_int_eq(op->removed_characters_list, 0);
}
END_TEST
#endif

void
free_operation(operation *op) {
    destroy_operation(op);
    free(op);
}

state_s*
new_state(void) {
    state_s *stp = g_malloc0(sizeof(state_s));
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
        .tried_chars = NULL,
    };
    *stp = temp;
}

void
destroy_state(state_s *stp) {
    free_instance(stp->instance);
    free_operation(stp->operation);
    if (stp->tried_chars != NULL)
        g_slist_free(stp->tried_chars);
}

void
free_state(state_s *stp) {
    destroy_state(stp);
    free(stp);
}

#ifdef TEST_EVERYTHING
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
    tcase_add_test(tc_core, destroy_instance_1);
    tcase_add_test(tc_core, copy_instance_1);
    tcase_add_test(tc_core, copy_instance_2);
    tcase_add_test(tc_core, new_operation_1);
    tcase_add_test(tc_core, destroy_operation_1);

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
