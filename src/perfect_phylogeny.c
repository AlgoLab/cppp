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
   Pretty print a state.
   Mainly used for debug
*/
void
log_state(const state_s* stp) {
#ifdef DEBUG
        log_debug("log_state");
        fprintf(stderr, "=======================================\n");
        fprintf(stderr, "State=");
        fprintf(stderr, "  num_species: %d\n", stp->num_species);
        fprintf(stderr, "  num_characters: %d\n", stp->num_characters);
        fprintf(stderr, "  num_species_orig: %d\n", stp->num_species_orig);
        fprintf(stderr, "  num_characters_orig: %d\n", stp->num_characters_orig);

        fprintf(stderr, "------|----------|------\n");
        fprintf(stderr, "      |          |      \n");
        fprintf(stderr, "  c   |characters|colors\n");
        fprintf(stderr, "------|----------|------\n");
        for (size_t i = 0; i < stp->num_characters_orig; i++)
                fprintf(stderr, "%6d|%10d|%6d\n", i,stp->characters[i], stp->colors[i]);
        fprintf(stderr, "------|-------|----------|------\n");

        fprintf(stderr, "------|-------\n");
        fprintf(stderr, "  s   |species\n");
        fprintf(stderr, "------|-------\n");
        for (size_t i = 0; i < stp->num_species_orig; i++) {
                fprintf(stderr, "%6d|%7d\n", i, stp->species[i]);
        }

        fprintf(stderr, "------|-------\n");

        fprintf(stderr, "  operation: %d\n", stp->operation);
        fprintf(stderr, "  realize: %d\n", stp->realize);

        fprintf(stderr, "connected_components: size %d\n", stp->red_black->num_vertices);
        log_array_uint32_t("connected_components", stp->connected_components, stp->red_black->num_vertices);

        log_array_bool("current_component", stp->current_component, stp->red_black->num_vertices);
        fprintf(stderr, "\n");


        log_state_lists(stp);
        log_state_graphs(stp);
#endif
}

void log_state_lists(const state_s* stp) {
#ifdef DEBUG
        log_debug("log_state_lists");
        log_array_uint32_t("  tried_characters", stp->tried_characters, stp->tried_characters_size);
        log_array_uint32_t("  character_queue", stp->character_queue, stp->character_queue_size);
#endif
}

void log_state_graphs(const state_s* stp) {
#ifdef DEBUG
        log_debug("log_state_graphs");
        fprintf(stderr, "  Red-black graph. Address\n", stp->red_black);
        graph_pp(stp->red_black);
        fprintf(stderr, "\n");

        fprintf(stderr, "  Conflict graph. Address\n", stp->conflict);
        graph_pp(stp->conflict);
        fprintf(stderr, "\n");
#endif
}

/**
   \brief some functions to abstract the access to the instance matrix
*/

static uint32_t
matrix_get_value(state_s *stp, uint32_t s, uint32_t c) {
        return stp->matrix[c + stp->num_characters * s];
}

static void
matrix_set_value(state_s *stp, uint32_t s, uint32_t c, uint32_t value) {
        stp->matrix[c + stp->num_characters * s] = value;
}


static uint32_t
state_cmp(const state_s *stp1, const state_s *stp2) {
        if (stp1->num_characters != stp2->num_characters)
                return 1;
        if (stp1->num_species != stp2->num_species)
                return 2;
        if (stp1->num_characters_orig != stp2->num_characters_orig)
                return 3;
        if (stp1->num_species_orig != stp2->num_species_orig)
                return 4;
        if (stp1->tried_characters_size > 0 && stp2->tried_characters_size >0 && stp1->tried_characters_size != stp2->tried_characters_size)
                return 41;
        if (stp1->character_queue_size > 0&& stp2->character_queue_size > 0 && stp1->character_queue_size != stp2->character_queue_size)
                return 42;
        if (stp1->operation != stp2->operation)
                return 43;
        if (stp1->realize != stp2->realize)
                return 44;
        if (stp1->backtrack_level != stp2->backtrack_level)
                return 45;

        if (stp1->species == NULL || stp2->species == NULL)
                return 7;
        if (memcmp(stp1->species, stp2->species, (stp2->num_species_orig) * sizeof((stp1->species)[0])) != 0)
                return 8;
        if (stp1->characters == NULL || stp2->characters == NULL)
                return 9;
        if (memcmp(stp1->characters, stp2->characters, (stp2->num_characters_orig) * sizeof((stp1->characters)[0])) != 0)
                return 10;
        if (stp1->character_queue_size > 0 && stp1->character_queue == NULL)
                return 12;
        if (stp2->character_queue_size > 0 && stp2->character_queue == NULL)
                return 13;
        if (stp1->character_queue_size > 0 && stp2->character_queue_size > 0 &&
            memcmp(stp1->character_queue, stp2->character_queue, (stp1->num_characters_orig) * sizeof((stp1->character_queue)[0])) != 0)
                return 14;

        if (stp1->tried_characters_size > 0 && stp1->tried_characters == NULL)
                return 61;
        if (stp2->tried_characters_size > 0 && stp2->tried_characters == NULL)
                return 62;
        if (stp1->tried_characters_size > 0 && stp2->tried_characters_size > 0 &&
            memcmp(stp1->tried_characters, stp2->tried_characters, (stp1->num_characters_orig) * sizeof((stp1->tried_characters)[0])) != 0)
                return 63;

        if (stp1->characters == NULL || stp2->characters == NULL)
                return 17;
        if (memcmp(stp1->colors, stp2->colors, (stp1->num_characters_orig) * sizeof((stp1->colors)[0])) != 0)
                return 18;

        if ((stp2->num_characters_orig) + (stp2->num_species_orig) != stp2->red_black->num_vertices)
                return 19;
        if (stp1->connected_components == NULL || stp2->connected_components == NULL)
                return 20;
        if (memcmp(stp1->connected_components, stp2->connected_components, ((stp1->num_characters_orig) + (stp1->num_species_orig)) * sizeof((stp1->connected_components)[0])) != 0)
                return 21;
        if (stp1->current_component == NULL || stp2->current_component == NULL)
                return 22;
        if (memcmp(stp2->current_component, stp2->current_component, ((stp2->num_characters_orig) + (stp2->num_species_orig)) * sizeof((stp2->current_component)[0])) != 0)
                return 23;

        if (stp1->matrix == NULL || stp2->matrix == NULL)
                return 22;
        if (memcmp(stp1->matrix, stp2->matrix, ((stp2->num_characters) * (stp2->num_species)) * sizeof((stp1->matrix)[0])) != 0)
                return 23;

        if (stp1->red_black == NULL || stp2->red_black == NULL)
                return 50;
        if (graph_cmp(stp1->red_black, stp2->red_black) != 0)
                return 51;
        if (stp1->conflict == NULL || stp2->conflict == NULL)
                return 52;
        if (graph_cmp(stp1->conflict, stp2->conflict) != 0)
                return 53;

        return 0;
}

void
copy_state(state_s* dst, const state_s* src) {
        assert(dst != NULL);
        log_debug("copy_state: input");
        check_state(src);

        dst->realize = src->realize;
        dst->num_species = src->num_species;
        dst->num_characters = src->num_characters;
        graph_copy(dst->red_black, src->red_black);
        graph_copy(dst->conflict, src->conflict);
        dst->matrix = src->matrix;
        assert(dst != NULL);

        assert(dst->characters != NULL);
        assert(dst->colors != NULL);
        assert(dst->species != NULL);
        memcpy(dst->characters, src->characters, src->num_characters_orig * sizeof(src->characters[0]));
        memcpy(dst->colors, src->colors, src->num_characters_orig * sizeof(src->colors[0]));
        memcpy(dst->species, src->species, src->num_species_orig * sizeof(src->species[0]));

        dst->operation = src->operation;

        assert(dst->connected_components != NULL);
        assert(dst->current_component != NULL);
        memcpy(dst->connected_components, src->connected_components, src->red_black->num_vertices * sizeof(src->connected_components[0]));
        memcpy(dst->current_component, src->current_component, src->red_black->num_vertices * sizeof(src->current_component[0]));

        dst->tried_characters_size = 0;
        dst->character_queue_size = 0;

        dst->backtrack_level = src->backtrack_level;
        assert(state_cmp(src, dst) == 0);
        log_debug("copy_state: return");
        check_state(dst);
        log_debug("Checking copy_state: %d", state_cmp(dst, src));
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
bool
realize_character(state_s* dst, state_s* src) {
        assert (src != NULL);
        assert (dst != NULL);
        assert (src != dst);
        log_debug("realize_character: dst=%p, src=%p character=%d", dst, src, src->realize);
        check_state(src);
        copy_state(dst, src);
        assert(state_cmp(src, dst) == 0);
        uint32_t character = src->realize;
        assert(src->characters[character]);
        uint32_t n = src->num_species_orig;

        log_debug("realize_character: Trying to realize CHAR %d", character);
        check_state(dst);
        uint32_t character_vertex = src->num_species_orig + character;
        assert(src->current_component[character_vertex]);
        uint32_t color = src->colors[character];
        log_array_bool("realize_character: src->current_component: ", src->current_component, src->red_black->num_vertices);
        log_debug("realize_character: check dst");
        log_debug("realize_character: color %d. Cases BLACK=>%d RED=>%d", color, (color == BLACK), (color == RED));
        check_state(dst);

        if (color == BLACK) {
                log_debug("realize_character: %d (vertex %d). inactive color %d = BLACK", character, character_vertex, color);
/*
  for each species s in the same connected component as c, delete the
  edge (s,c) if it exists and create the edge (s,c) if it does not exist
*/
                for (uint32_t v=0; v<n; v++)
                        if (src->current_component[v])
                                if (graph_get_edge(src->red_black, character_vertex, v) && character_vertex != v)
                                        graph_del_edge(dst->red_black, character_vertex, v);
                                else
                                        graph_add_edge(dst->red_black, character_vertex, v);

                src->operation = 1;
                dst->colors[character] = RED;
        }
        if (color == RED) {
                log_debug("realize_character: %d (vertex %d). active. color %d = RED", character, character_vertex, color);
/*
  if there is a species in the same connected component as c, but that
  it is not adjacent to c, then the realization is impossible.

  If c is adjacent to all species in its connected component, remove
  all edges incident on c, because c is free.
*/
                for (uint32_t v=0; v<n; v++)
                        if (src->current_component[v])
                                if (graph_get_edge(src->red_black, character_vertex, v) && character_vertex != v) {
                                        src->operation = 2;
                                        dst->colors[character] = RED + 1;
                                        graph_del_edge(dst->red_black, character_vertex, v);
                                } else {
                                        src->operation = 0;
                                        log_debug("realize_character: end. REALIZATION IMPOSSIBLE");
                                        return false;
                                }
        }

        dst->realize = character;
        log_debug("realize_character: before cleanup");
        check_state(dst);
        cleanup(dst);
        check_state(dst);
        log_debug("realize_character: call update_connected_components");
        update_connected_components(dst);
        check_state(dst);
        log_debug("realize_character: update_conflict_graph");
        update_conflict_graph(dst);
        check_state(dst);
        log_debug("realize_character: color %d", color);
        log_debug("realize_character: outcome %d (1=>activated, 2=>freed)", dst->operation);
        log_debug("realize_character: return");
        check_state(dst);
        return true;
}

/**
   \brief read the file containing an instance of the ppp problem and computes the
   corresponding state
   \param filename stp

   \c stp is a pointer to an existing state

   Reads an instance from file. If \c global_props contains a \c NULL \c file,
   then also the first row of the file, storing the number of species and
   characters must be read.
   If the file contains no instances to be read, then the function returns \c NULL.

   Updates an instance by computing the red-black and the conflict graphs
   associated to a given matrix.

   In a red-black graph, the first \c stp->num_species ids correspond to species,
   while the ids larger or equal to stp->num_species correspond to characters.
   Notice that the label id must be conserved when modifying the graph (i.e.
   realizing a character).

   color attribute is \c SPECIES if the vertex is a species, otherwise it is \c BLACK
   or \c RED (at the beginning, there can only be \c BLACK edges).

*/
bool
read_instance_from_filename(instances_schema_s* global_props, state_s* stp) {
        assert(global_props->filename != NULL);
        log_debug("Reading data from:%s\n", global_props->filename);
        if (global_props->file == NULL) {
                global_props->file = fopen(global_props->filename, "r");
                assert(global_props->file != NULL);
                if (feof(global_props->file))
                        error(3, 0, "Could not open input file: %s\n", global_props->filename);

                int err = fscanf(global_props->file, "%"SCNu32" %"SCNu32, &(global_props->num_species),
                                 &(global_props->num_characters));
                if (err == EOF)
                        error(1, 0, "Could not read the first line of file: %s\n", global_props->filename);
        }

        init_state(stp, global_props->num_species, global_props->num_characters);
        stp->num_species = global_props->num_species;
        stp->num_characters = global_props->num_characters;
        stp->matrix = GC_MALLOC_ATOMIC(stp->num_species * stp->num_characters * sizeof(uint32_t));
        assert(stp->matrix != NULL);
        for(uint32_t s=0; s < stp->num_species; s++)
                for(uint32_t c=0; c < stp->num_characters; c++) {
                        uint32_t x = -1;
                        int err = fscanf(global_props->file, "%"SCNu32, &x);
/*
  Check that the file is not ended in the middle of an instance
*/
                        if (err == EOF && (s != 0 || c != 0))
                                error(2, 0, "Badly formatted input file: %s\n", global_props->filename);
                        if (feof(global_props->file)) {
                                log_debug("Read instance: EOF");
                                fclose(global_props->file);
                                return false;
                        }
                        matrix_set_value(stp, s, c, x);
                }
#ifdef DEBUG
        log_debug("MATRIX");
        for(uint32_t s=0; s < stp->num_species; s++) {
                for(uint32_t c=0; c < stp->num_characters; c++)
                        fprintf(stderr, "%d", matrix_get_value(stp, s, c));
                fprintf(stderr, "\n");
        }
#endif
        /* red-black graph */
        for(uint32_t s=0; s < stp->num_species; s++)
                for(uint32_t c=0; c < stp->num_characters; c++)
                        if (matrix_get_value(stp, s, c) == 1)
                                graph_add_edge(stp->red_black, s, c + stp->num_species);
#ifdef DEBUG
        log_debug("MATRIX");
        graph_pp(stp->red_black);
#endif
        /* check the red-black graph */
        for(uint32_t s=0; s < stp->num_species; s++)
                for(uint32_t c=0; c < stp->num_characters; c++)
                        assert(matrix_get_value(stp, s, c) == 0 && !graph_get_edge(stp->red_black, s, c + stp->num_species) ||
                               matrix_get_value(stp, s, c) == 1 && graph_get_edge(stp->red_black, s, c + stp->num_species));
        update_connected_components(stp);
        check_state(stp);
        cleanup(stp);
        check_state(stp);
        log_debug("read_instance_from_filename: call update_connected_components");
        update_connected_components(stp);
        check_state(stp);
        log_debug("read_instance_from_filename: update_conflict_graph");
        update_conflict_graph(stp);

        memset(stp->tried_characters, 0, stp->num_characters_orig * sizeof((stp->tried_characters)[0]));
        memset(stp->character_queue, 0, stp->num_characters_orig * sizeof((stp->character_queue)[0]));
        stp->tried_characters_size = 0;
        stp->character_queue_size = 0;
        stp->operation = 0;
        stp->realize = 0;
        stp->backtrack_level = 0;

        log_state(stp);
        log_debug("read_instance_from_filename: completed");
        return true;
}


/*
  \brief Simplify the instance whenever possible.

  We remove null characters and species.
*/
void cleanup(state_s *stp) {
        assert(stp != NULL);
        log_debug("cleanup");
        log_state(stp);
        // Looking for null species
        for (uint32_t s=0; s < stp->num_species_orig; s++)
                if (stp->species[s] && graph_degree(stp->red_black, s) == 0) {
                        log_debug("Want to delete species %d\n", s);
                        delete_species(stp, s);
                }
// Looking for null characters
        for (uint32_t c = 0; c < stp->num_characters_orig; c++)
                if (stp->characters[c] && graph_degree(stp->red_black, c + stp->num_species_orig) == 0) {

                        log_debug("Want to delete character %d\n", c);
                        delete_character(stp, c);
                }
/* TODO (if necessary) */
/* we remove duplicated characters */
/* we remove duplicated species */
        log_debug("cleanup: final state");
        log_state(stp);
        log_debug("cleanup: end");
}

graph_s *
get_red_black_graph(const state_s *inst) {
        // TODO
        return NULL;
}

graph_s *
get_conflict_graph(const state_s *inst) {
        // TODO
        return NULL;
}


void
init_state(state_s *stp, uint32_t n, uint32_t m) {
        log_debug("init_state n=%d m=%d", n, m);
        assert(stp != NULL);
        stp->num_characters_orig = m;
        stp->num_species_orig = n;
        stp->num_characters = m;
        stp->num_species = n;
        stp->realize = 0;
        stp->species = xmalloc(n * sizeof(bool));
        stp->characters = xmalloc(m * sizeof(bool));
        stp->colors = xmalloc(m * sizeof(uint8_t));

        stp->tried_characters = xmalloc(m * sizeof(uint32_t));
        stp->character_queue = xmalloc(m * sizeof(uint32_t));
        stp->connected_components = xmalloc((m + n) * sizeof(uint32_t));
        stp->current_component = xmalloc((m + n) * sizeof(bool));
        stp->operation = 0;

        stp->red_black = graph_new(n + m);
        assert(stp->red_black != NULL);
        stp->conflict = graph_new(m);
        assert(stp->conflict != NULL);

        for (uint32_t i=0; i < n; i++) {
                stp->species[i] = true;
        }

        for (uint32_t i=0; i < m; i++) {
                stp->tried_characters[i] = -1;
                stp->character_queue[i] = -1;
                stp->characters[i] = true;
                stp->colors[i] = BLACK;
        }
        stp->character_queue_size = 0;
        stp->tried_characters_size = 0;


        log_debug("init_state: before update_connected_components");
        update_connected_components(stp);
        log_debug("init_state: completed");
        check_state(stp);
}

void
check_state(const state_s* stp) {
        uint32_t err = 0;
#ifdef DEBUG
        if (stp->num_species == -1 || stp->num_species > stp->num_species_orig) {
                err = 1;
                log_debug("check_state error: Line %d (%d != %d)", __LINE__, stp->num_species, stp->num_species_orig);
        }
        if (stp->num_characters == -1 || stp->num_characters > stp->num_characters_orig) {
                err = 2;
                log_debug("check_state error: Line %d (%d != %d)", __LINE__, stp->num_characters, stp->num_characters_orig);
        }

        uint32_t count = 0;
        for (uint32_t s = 0; s < stp->num_species_orig; s++) {
                if (stp->species[s])
                        count++;
        }
        if (count != stp->num_species) {
                err = 3;
                log_debug("check_state error: Line %d (%d != %d)", __LINE__, stp->num_species, count);
        }

        count = 0;
        for (uint32_t c = 0; c < stp->num_characters_orig; c++) {
                if (stp->characters[c])
                        count++;
        }
        if (count != stp->num_characters) {
                err = 4;
                log_debug("check_state error: Line %d (%d != %d)", __LINE__, stp->num_characters, count);
        }

        if (count != stp->num_characters) {
                err = 5;
                log_debug("Line %d (%d != %d)", __LINE__, stp->num_characters, count);
        }

        // check connected_components
        uint32_t max_conn = 0;
        for (uint32_t v = 0; v < stp->red_black->num_vertices; v++)
                if ((stp->connected_components)[v] > max_conn)
                        max_conn = (stp->connected_components)[v];

        bool component_id[max_conn + 1];
        memset(component_id, 0, (max_conn + 1) * sizeof(bool));
        for (uint32_t v = 0; v < stp->red_black->num_vertices; v++)
                component_id[stp->connected_components[v]] = true;
        for (uint32_t c = 0; c <= max_conn; c++)
                if (!component_id[c]) {
                        err = 6;
                        log_debug("Line %d %d %d", __LINE__, c, component_id[c]);
                }
        assert(stp->red_black != NULL);

        if ((stp->num_characters_orig) + (stp->num_species_orig) != stp->red_black->num_vertices) {
                err = 7;
                log_debug("Line %d (%d + %d != %d)", __LINE__, stp->num_characters_orig, stp->num_species_orig, stp->red_black->num_vertices);
        }

#endif
        if (err > 0) {
                log_state(stp);
                log_debug("check_graph code: %d", err);
        }
        assert(err == 0);
        graph_check(stp->red_black);
        graph_check(stp->conflict);
}

uint32_t
characters_list(state_s * stp, uint32_t *array) {
        assert(array != NULL);
        uint32_t size = 0;
        for (uint32_t c=0; c < stp->num_characters_orig; c++)
                if (stp->colors[c] == RED)
                        array[size++] = c;
        for (uint32_t c=0; c < stp->num_characters_orig; c++)
                if (stp->colors[c] == BLACK)
                        array[size++] = c;
        return size;
}

void
delete_character(state_s *stp, uint32_t c) {
        log_debug("Deleting character %d", c);
        assert(c < stp->num_characters_orig);
        assert(stp->characters[c]);
        assert(stp->colors[c] > 0);
        stp->characters[c] = false;
        (stp->num_characters)--;
}

void
delete_species(state_s *stp, uint32_t s) {
        log_debug("Deleting species %d", s);
        assert(s < stp->num_species_orig);
        assert(stp->species[s] > 0);
        stp->species[s] = false;
        (stp->num_species)--;
}



void
smallest_component(state_s* stp) {
        assert(stp != NULL);
        assert(stp->connected_components != NULL);
        log_debug("smallest_component. stp=%p", stp);
        log_array_uint32_t("stp->connected_components", stp->connected_components, stp->red_black->num_vertices);
        stp->character_queue_size = stp->red_black->num_vertices + 1;
/**
   We need only the connected components that contain at least a
   species and a character.
   We only have to count the number of characters contained in the
   component.

   The first character in \c character_queue is the one with the
   largest degree in the red-black graph, since it is the most likely
   to be realized first.
   Moreover, when the instance has no conflict, we simulate the
   standard algorithm to compute the perfect phylogeny.
*/
        uint32_t card[stp->red_black->num_vertices];
        memset(card, 0, stp->red_black->num_vertices * sizeof(card[0]));
        for (uint32_t w = 0; w < stp->red_black->num_vertices; w++)
                card[stp->connected_components[w]] += 1;
        uint32_t smallest_component = stp->red_black->num_vertices + 1;
        uint32_t smallest_size = stp->red_black->num_vertices + 1;
        for (uint32_t w = 0; w < stp->num_species_orig + stp->num_characters_orig; w++)
                if (card[w] > 1 && card [w] < smallest_size) {
                        smallest_size = card[w];
                        smallest_component = w;
                }

        log_debug("smallest_component: %d smallest_size: %d", smallest_component, smallest_size);
        for (uint32_t w = 0; w < stp->red_black->num_vertices; w++)
                stp->current_component[w] = (stp->connected_components[w] == smallest_component);
        uint32_t p = 0;
        uint32_t maximum_char = 0;
        uint32_t max_degree = 0;
        for (uint32_t w = stp->num_species_orig; w < stp->num_species_orig + stp->num_characters_orig; w++)
                if (stp->connected_components[w] == smallest_component) {
                        if (graph_degree(stp->red_black, w) > max_degree) {
                                max_degree = graph_degree(stp->red_black, w);
                                maximum_char = p;
                        }
                        stp->character_queue[p++] = w - stp->num_species_orig;
                }
        stp->character_queue_size = p;
        log_array_uint32_t("card: ", card, stp->red_black->num_vertices);
        log_debug("maximum_char: %d max_degree: %d", maximum_char, max_degree);
        log_array_uint32_t("character_queue", stp->character_queue, stp->character_queue_size);
/* Put the character with maximum degree in front of
   stp->character_queue */

        if (maximum_char > 0) {
                uint32_t temp = stp->character_queue[0];
                stp->character_queue[0] = stp->character_queue[maximum_char];
                stp->character_queue[maximum_char] = temp;
        }
        log_debug("character_queue_size: %d", stp->character_queue_size);
        log_array_uint32_t("character_queue", stp->character_queue, stp->character_queue_size);
        log_debug("smallest_component: end");
}

void
update_conflict_graph(state_s* stp) {
        log_debug("update_conflict_graph");
        graph_pp(stp->conflict);
        graph_nuke_edges(stp->conflict);
        log_debug("update_conflict_graph: nuked edges");
        graph_pp(stp->conflict);
        for(uint32_t c1 = 0; c1 < stp->num_characters; c1++)
                for(uint32_t c2 = c1 + 1; c2 < stp->num_characters; c2++) {
                        uint32_t states[2][2] = { {0, 0}, {0, 0} };
                        for(uint32_t s=0; s < stp->num_species; s++)
                                states[matrix_get_value(stp, s, c1)][matrix_get_value(stp, s, c2)] = 1;
                        if(states[0][0] + states[0][1] + states[1][0] + states[1][1] == 4)
                                graph_add_edge(stp->conflict, c1, c2);
                }
        log_debug("update_conflict_graph: end");
        graph_pp(stp->conflict);
}

void
update_connected_components(state_s* stp) {
        log_debug("update_connected_components. stp=%p", stp);
        connected_components(stp->red_black, stp->connected_components);
        log_array_uint32_t("stp->connected_components", stp->connected_components, stp->red_black->num_vertices);
        log_debug("update_connected_components: end");
}
