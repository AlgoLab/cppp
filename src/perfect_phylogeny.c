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
        instp->current = GC_MALLOC(nchars * sizeof(uint32_t));
        instp->species = GC_MALLOC(nchars * sizeof(uint32_t));
        instp->characters = GC_MALLOC(nchars * sizeof(uint32_t));
        instp->operation = 0;
        for (uint32_t i=0; i < nspecies; i++) {
                instp->species[i] = 1;
        }
        for (uint32_t i=0; i < nchars; i++) {
                instp->current[i] = 0;
                instp->characters[i] = 1;
        }
}

/**
   \brief some functions to abstract the access to the instance matrix
*/

static uint32_t
matrix_get_value(pp_instance *inst, uint32_t species, uint32_t character) {
        return inst->matrix[character + inst->num_characters * species];
}

static void
matrix_set_value(pp_instance *inst, uint32_t species, uint32_t character, uint32_t value) {
        inst->matrix[character + inst->num_characters * species] = value;
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

        for(uint32_t s=0; s < instp->num_species; s++) {
                SETVAN(instp->red_black, "id", s, s);
                SETVAN(instp->red_black, "color", s, SPECIES);
        }
        for(uint32_t c=0; c < instp->num_characters; c++) {
                SETVAN(instp->red_black, "id", c + instp->num_species, c);
                SETVAN(instp->red_black, "color", c + instp->num_species, BLACK);
        }
        for (uint32_t s=0; s<instp->num_species; s++)
                for (uint32_t c=0; c<instp->num_characters; c++)
                        if (matrix_get_value(instp, s, c) == 1)
                                igraph_add_edge(instp->red_black, s, c+instp->num_species);

        /* Now we compute the conflict graph */
        for(uint32_t c1 = 0; c1 < instp->num_characters; c1++)
                for(uint32_t c2 = c1 + 1; c2 < instp->num_characters; c2++) {
                        uint32_t states[2][2] = { {0, 0}, {0, 0} };
                        for(uint32_t s=0; s<instp->num_species; s++)
                                states[matrix_get_value(instp, s, c1)][matrix_get_value(instp, s, c2)] = 1;
                        if(states[0][0] + states[0][1] + states[1][0] + states[1][1] == 4)
                                igraph_add_edge(instp->conflict, c1, c2);
                }
}





void copy_state(state_s* dst, const state_s* src) {
        assert(dst != NULL);
        if (dst->instance == NULL) dst->instance = new_instance();
        copy_instance(dst->instance, src->instance);
        dst->realized_char = src->realized_char;
        dst->tried_characters = g_slist_copy(src->tried_characters);
        dst->character_queue = g_slist_copy(src->character_queue);
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
realize_character(const pp_instance src, const uint32_t character) {
        pp_instance dst;
        copy_instance(&dst, &src);
        igraph_integer_t c = (igraph_integer_t) src.num_species_orig + character;
        int ret = 0;

        igraph_vector_t conn_comp;
        igraph_vector_init(&conn_comp, 1);
        ret = igraph_subcomponent(dst.red_black, &conn_comp, c, IGRAPH_ALL);
        assert(ret == 0);
        igraph_vector_sort(&conn_comp);

        igraph_vector_t adjacent;
        igraph_vector_init(&adjacent, 1);
        ret = igraph_neighbors(dst.red_black, &adjacent, c, IGRAPH_ALL);
        assert(ret == 0);
        igraph_vector_t not_adjacent;
        igraph_vector_init(&not_adjacent, 0);
        igraph_vector_t temp;
        igraph_vector_init(&temp, 1);
        igraph_vector_difference_sorted(&conn_comp, &adjacent, &temp);
        igraph_vector_t new_red;
        size_t l=igraph_vector_size(&temp);
        igraph_vector_init(&new_red, 0);
        for (size_t i=0; i<l; i++) {
                uint32_t v = VECTOR(temp)[i];
                /* check if v is a species */
                if (v < dst.num_species && v != c) {
                        igraph_vector_push_back(&new_red, c);
                        igraph_vector_push_back(&new_red, v);
                        igraph_vector_push_back(&not_adjacent, v);
                }
        }
        int color = VAN(dst.red_black, "color", c);
        assert(color != SPECIES);
        igraph_es_t es;
        igraph_es_incident(&es, c, IGRAPH_ALL);
        igraph_delete_edges(dst.red_black, es);
        igraph_es_destroy(&es);
        /* igraph_vector_print(&adjacent); */
        /* igraph_vector_print(&not_adjacent); */
        g_debug("CHAR %d\n", character);
        if (color == BLACK) {
                igraph_add_edges(dst.red_black, &new_red, 0);
                dst.operation = 1;
                SETVAN(dst.red_black, "color", c, RED);
                dst.current[character] = 1;
        }
        if (color == RED) {
                /* igraph_vector_print(&adjacent); */
                if (igraph_vector_size(&not_adjacent) > 0) {
                        dst.operation = -1;
                } else {
                        dst.operation = 2;
                        SETVAN(dst.red_black, "color", c, RED + 1);
                        delete_character(&dst, character);
                }
        }
        /* igraph_write_graph_edgelist(dst.red_black, stdout); */
        instance_cleanup(&dst);
        igraph_vector_destroy(&new_red);
        igraph_vector_destroy(&temp);
        igraph_vector_destroy(&not_adjacent);
        igraph_vector_destroy(&adjacent);
        igraph_vector_destroy(&conn_comp);
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

  We remove null characters and species.
*/
void instance_cleanup(pp_instance *instp) {
        g_debug("Cleanup\n");
        // Looking for null species
        for (uint32_t s=0; s < instp->num_species_orig; s++)
                if (instp->species[s]) {
                        igraph_es_t es;
                        igraph_integer_t size;
                        igraph_es_incident(&es, s, IGRAPH_ALL);
                        igraph_es_size(instp->red_black, &es, &size);
                        if (size == 0)
                                delete_species(instp, s);
                        igraph_es_destroy(&es);
                }

        // Looking for null characters
        for (uint32_t c = 0; c < instp->num_characters_orig; c++)
                if (instp->characters[c]) {
                        igraph_es_t es;
                        igraph_integer_t size;
                        igraph_es_incident(&es, instp->num_species_orig + c, IGRAPH_ALL);
                        igraph_es_size(instp->red_black, &es, &size);
                        if (size == 0)
                                delete_character(instp, c);
                        igraph_es_destroy(&es);
                }
/* TODO (if necessary) */
/* we remove duplicated characters */
/* we remove duplicated species */
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
        if (conflict != NULL) {
                for (uint32_t c1=0; c1<inst.num_characters; c1++)
                        for (uint32_t c2=0; c2<inst.num_characters; c2++) {
                                igraph_integer_t eid;
                                igraph_get_eid(inst.conflict, &eid, c1, c2, 0, 0);
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
        ck_assert_msg(instp->current == NULL, "instp->current has not been freed\n");
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
        memcpy(dst->current, src->current, src->num_characters_orig * sizeof(uint32_t));
        memcpy(dst->species, src->species, src->num_species_orig * sizeof(uint32_t));
        memcpy(dst->characters, src->characters, src->num_characters_orig * sizeof(uint32_t));
        dst->operation = src->operation;
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
				 "  current: %p\n"
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
				 (void *) instp->current,
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
        /* free(instp->current); */
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



state_s*
new_state(void) {
        state_s *stp = GC_MALLOC(sizeof(state_s));
        init_state(stp);
        return stp;
}

void
init_state(state_s *stp) {
        assert(stp != NULL);
        stp->instance = new_instance();
        stp->realized_char = 0;
        stp->tried_characters = NULL;
        stp->character_queue = NULL;
}

void
destroy_state(state_s *stp) {
        /* free_instance(stp->instance); */
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
        return err;
}

static uint32_t check_instance(const pp_instance* instp) {
        uint32_t err = 0;
        if (instp->num_species == -1 || instp->num_species > instp->num_species_orig) {
                err += 1;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %d)", instp->num_species, 0);
        }
        if (instp->num_characters == -1 || instp->num_characters > instp->num_characters_orig) {
                err += 2;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %d)", instp->num_characters, 0);
        }

        uint32_t count = 0;
        for (uint32_t s = 0; s < instp->num_species_orig; s++) {
                if (instp->species[s])
                        count++;
        }
        if (count != instp->num_species) {
                err += 4;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %d)", instp->num_species, count);
        }

        count = 0;
        for (uint32_t c = 0; c < instp->num_characters_orig; c++) {
                if (instp->characters[c])
                        count++;
        }
        if (count != instp->num_characters) {
                err += 8;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %d)", instp->num_characters, count);
        }

        count = 0;
        for (uint32_t c = 0; c < instp->num_characters_orig; c++) {
                if (instp->current[c] != -1)
                        count++;
        }
        if (count != instp->num_characters) {
                err += 16;
                g_debug("__FUNCTION__@__FILE__: __LINE__ (%d != %d)", instp->num_characters, count);
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
        pp_instance* instp = new_instance();
        stp->instance = instp;

        json_error_t jerr;
        json_t* data = json_load_file(filename, JSON_DISABLE_EOF_CHECK, &jerr);
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
                stp->instance->current = json_get_array(instpj, "current");
                stp->instance->species = json_get_array(instpj, "species");
                stp->instance->characters = json_get_array(instpj, "characters");

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
                assert(!igraph_write_graph_graphml(stp->instance->red_black, fp, true));
                fclose(fp);
                assert(!json_object_set(instp, "red_black_file", json_string(g_filename)));
        }

        if (stp->instance->conflict != NULL) {
                asprintf(&g_filename, "%s-conflict.graphml", filename);
                fp = fopen(g_filename, "w");
                assert(!igraph_write_graph_graphml(stp->instance->conflict, fp, true));
                fclose(fp);
                assert(!json_object_set(instp, "conflict_file", json_string(g_filename)));
        }
        free(g_filename);

        if (stp->instance->matrix != NULL)
                assert(!json_object_set(instp, "matrix", array2json_array(stp->instance->matrix, stp->instance->num_species * stp->instance->num_characters)));
        assert(!json_object_set(instp, "current", array2json_array(stp->instance->current, stp->instance->num_characters_orig)));
        assert(!json_object_set(instp, "species", array2json_array(stp->instance->species, stp->instance->num_species_orig)));
        assert(!json_object_set(instp, "characters", array2json_array(stp->instance->characters, stp->instance->num_characters_orig)));

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
        stp->realized_char = 0;
        stp->tried_characters = NULL;
        stp->character_queue = NULL;
}


GSList* characters_list(state_s * stp) {
        GSList* list = NULL;
        for (uint32_t c=0; c < stp->instance->num_characters_orig; c++)
                if (stp->instance->characters[c])
                        list = g_slist_prepend(list, GINT_TO_POINTER(c));
        return g_slist_reverse(list);
}


void delete_species(pp_instance *instp, uint32_t s) {
        g_debug("Deleting species %d\n", s);
        assert(s < instp->num_species_orig);
        assert(instp->species[s] > 0);
        instp->species[s] = 0;
        (instp->num_species)--;
}

void delete_character(pp_instance *instp, uint32_t c) {
        g_debug("Deleting character %d\n", c);
        assert(c < instp->num_characters_orig);
        assert(instp->characters[c] > 0);
        assert(instp->current[c] != -1);
        instp->characters[c] = 0;
        instp->current[c] = -1;
        (instp->num_characters)--;
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
        pp_instance inst2 = realize_character(*stp->instance, 0);
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

/* static int g_slist_cmp(GSList* l1, GSList* l2) { */
/*         if (l1 == NULL && l2 == NULL) return 0; */
/*         if (l1 == NULL) return 1; */
/*         if (l2 == NULL) return -1; */
/*         GSList* x1 = g_slist_next(l1); */
/*         GSList* x2 = g_slist_next(l2); */
/*         for (;x1 != NULL && x2 != NULL; x1 = g_slist_next(x1), x2 = g_slist_next(x2)) { */
/*                 int32_t d = GPOINTER_TO_INT(x2->data) - GPOINTER_TO_INT(x1->data); */
/*                 if (d != 0) return d; */
/*         } */
/*         if (x1 == NULL && x2 == NULL) return 0; */
/*         return (x1 == NULL) ? 1 : -1; */
/* } */
static uint32_t instance_cmp(pp_instance *instp1, pp_instance *instp2) {
        uint32_t result = 0;
        if (instp1->num_characters != instp2->num_characters) result += 1;
        if (instp1->num_species != instp2->num_species) result += 2;
        if (instp1->current == NULL || instp2->current == NULL ||
		memcmp(instp1->current, instp2->current, sizeof(*(instp1->current)))) result += 4;
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

        tcase_add_test(tc_core, test_read_instance_from_filename_3);
/* tcase_add_test(tc_core, write_json_1); */
/* tcase_add_test(tc_core, write_json_2); */
        tcase_add_test(tc_core, write_json_3);
        tcase_add_test(tc_core, realize_3_0);

        suite_add_tcase(s, tc_core);

        return s;
}

/**
   This file is mainly used as a library.

   The standalone file is used for testing. If it is invoked without arguments,
   it runs a battery of unit tests, defined with the *check* package.
   It is invoked with a filename argument, the filename is a json file
   describing the regression test.

   If the json file contains an empty list of characters to be realized, then
   cleanup the instance.
*/
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
        json_error_t error;
        json_t* data = json_load_file(argv[1], 0, &error);
        assert(data != NULL && "Could not parse JSON file\n");
        unsigned int test_type = json_integer_value(json_object_get(data,"test"));
        if (test_type == 1) {
                const char *input_json_filename = json_string_value(json_object_get(data,"input"));
                json_t* listc = json_object_get(data, "characters");
                size_t index;
                json_t *value;
                state_s *stp = read_state(input_json_filename);
                assert(check_instance(stp->instance) == 0);
                assert(check_state(stp) == 0);
                if (json_array_size(listc) > 0)
                        json_array_foreach(listc, index, value) {
                                pp_instance inst2 = realize_character(*stp->instance, json_integer_value(value));
                                stp->instance = &inst2;
                                assert(check_instance(stp->instance) == 0);
                                assert(check_state(stp) == 0);
                        }
                else
                        instance_cleanup(stp->instance);
                assert(check_instance(stp->instance) == 0);
                assert(check_state(stp) == 0);
                write_state(json_string_value(json_object_get(data,"output")), stp);
        }
        return EXIT_SUCCESS;
}
#endif
