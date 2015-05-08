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
void log_state(const state_s* stp) {
        // return immediately if no log should be output
        if (!log_debug("log_state"))
                return;
        fprintf(stderr, "=======================================\n");
        fprintf(stderr, "State=");
        if (!check_state(stp)) fprintf(stderr, "NOT ");
        fprintf(stderr, "ok\n");
        fprintf(stderr, "  num_species: %d\n", stp->num_species);
        fprintf(stderr, "  num_characters: %d\n", stp->num_characters);
        fprintf(stderr, "  num_species_orig: %d\n", stp->num_species_orig);
        fprintf(stderr, "  num_characters_orig: %d\n", stp->num_characters_orig);

        fprintf(stderr, "------|-------|----------|------\n");
        fprintf(stderr, "      |current|          |      \n");
        fprintf(stderr, "  c   |states |characters|colors\n");
        fprintf(stderr, "------|-------|----------|------\n");
        for (size_t i = 0; i < stp->num_characters_orig; i++)
                fprintf(stderr, "%6d|%7d|%10d|%6d\n", i, stp->current_states[i], stp->characters[i], stp->colors[i]);
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

        log_state_lists(stp);
        log_state_graphs(stp);
}

void log_state_lists(const state_s* stp) {
        // return immediately if no log should be output
        if (!log_debug("log_state_lists"))
                return;
        fprintf(stderr, "  tried_characters. Size %d  Address %p Values: ", stp->tried_characters_size, stp->tried_characters);
        for(uint32_t i = 0; i < stp->tried_characters_size; i++)
                fprintf(stderr, "%d ", stp->tried_characters[i]);
        fprintf(stderr, "\n");

        fprintf(stderr, "  character_queue. Size %d Address %p Values: ", stp->character_queue_size, stp->character_queue);
        for(uint32_t i = 0; i < stp->character_queue_size; i++)
                fprintf(stderr, "%d ", stp->character_queue[i]);
        fprintf(stderr, "\n");
}

void log_state_graphs(const state_s* stp) {
        // return immediately if no log should be output
        if (!log_debug("log_state_graphs"))
                return;
        fprintf(stderr, "  Red-black graph. Address\n", stp->red_black);
        graph_pp(stp->red_black);
        fprintf(stderr, "\n");

        fprintf(stderr, "  Conflict graph. Address\n", stp->conflict);
        graph_pp(stp->conflict);
        fprintf(stderr, "\n");
}

/**
   \brief some functions to abstract the access to the instance matrix
*/

static uint32_t
matrix_get_value(state_s *stp, uint32_t species, uint32_t character) {
        return stp->matrix[character + stp->num_characters * species];
}

static void
matrix_set_value(state_s *stp, uint32_t species, uint32_t character, uint32_t value) {
        stp->matrix[character + stp->num_characters * species] = value;
}



static uint32_t state_cmp(const state_s *stp1, const state_s *stp2) {
        uint32_t result = 0;
        if (stp1->num_characters != stp2->num_characters) result += 1;
        if (stp1->num_species != stp2->num_species) result += 2;
        if (stp1->num_characters_orig != stp2->num_characters_orig) result += 4;
        if (stp1->num_species_orig != stp2->num_species_orig) result += 8;
        if (stp1->current_states == NULL || stp2->current_states == NULL)  result += 16;
        else for (size_t i = 0; i < stp2->num_characters_orig; i++)
                     if (stp1->current_states[i] != stp2->current_states[i]) {
                             result += 16;
                             break;
                     }
        if (stp1->species == NULL || stp2->species == NULL)  result += 32;
        else for (size_t i = 0; i < stp2->num_species_orig; i++)
                     if (stp1->species[i] != stp2->species[i]) {
                             result += 32;
                             break;
                     }
        if (stp1->characters == NULL || stp2->characters == NULL)  result += 64;
        else for (size_t i = 0; i < stp2->num_characters_orig; i++)
                     if (stp1->characters[i] != stp2->characters[i]) {
                             result += 64;
                             break;
                     }
        return result;
}

void
full_copy_state(state_s* dst, const state_s* src) {
        copy_state(dst, src);
        memcpy(dst->character_queue, src->character_queue, src->num_characters_orig * sizeof(src->character_queue[0]));
        memcpy(dst->tried_characters, src->tried_characters, src->num_characters_orig * sizeof(src->tried_characters[0]));
}


void
copy_state(state_s* dst, const state_s* src) {
        assert(dst != NULL);
        assert(check_state(src));
        dst->realize = src->realize;
        dst->num_species = src->num_species;
        dst->num_characters = src->num_characters;
        graph_copy(dst->red_black, src->red_black);
        graph_copy(dst->conflict, src->conflict);
        dst->matrix = src->matrix;
        memcpy(dst->current_states, src->current_states, src->num_characters_orig * sizeof(src->current_states[0]));
        memcpy(dst->characters, src->characters, src->num_characters_orig * sizeof(src->characters[0]));
        memcpy(dst->colors, src->colors, src->num_characters_orig * sizeof(src->colors[0]));
        memcpy(dst->species, src->species, src->num_species_orig * sizeof(src->species[0]));
        dst->operation = src->operation;
        dst->character_queue = NULL;
        dst->tried_characters = NULL;
        assert(check_state(dst));
        assert(state_cmp(dst, src) == 0);
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
realize_character(state_s* dst, const state_s* src) {
        assert (src != NULL);
        assert (dst != NULL);
        assert (src != dst);
        copy_state(dst, src);
        assert(state_cmp(src, dst) == 0);
        assert(check_state(src));
        uint32_t character = src->realize;
        uint32_t n = src->num_species_orig;

        log_debug("Trying to realize CHAR %d", character);
        assert(check_state(dst));
        uint32_t c = src->num_species_orig + character;
        int color = src->colors[character];
        bool conn_comp[src->num_species_orig + src->num_characters_orig];
        graph_reachable(dst->red_black, c, conn_comp);

        assert(check_state(dst));
        if (color == BLACK) {
                log_debug("color %d = BLACK", color);
                /*
                  for each species s in the same connected component
                  as c, delete the edge (s,c) if it exists and
                  create the edge (s,c) if it does not exist
                */
                for (uint32_t v=0; v<n; v++)
                        if (conn_comp[v])
                                if (graph_edge_p(dst->red_black, c, v))
                                        graph_del_edge(dst->red_black, c, v);
                                else
                                        graph_add_edge(dst->red_black, c, v);
                dst->operation = 1;
                dst->colors[character] = RED;
                dst->current_states[character] = 1;
        }
        if (color == RED) {
                log_debug("color %d = RED", color);
                dst->operation = 2;
                dst->colors[character] = RED + 1;
                /*
                  if there is a species in the same connected
                  component as c, but that it is not adjacent to c,
                  then the realization is impossible.

                  If c is adjacent to all species in its connected
                  component, remove all edges incident on c, because c
                  is free.
                */
                for (uint32_t v=0; v<n; v++)
                        if (conn_comp[v])
                                if (graph_edge_p(dst->red_black, c, v))
                                        graph_del_edge(dst->red_black, c, v);
                                else {
                                        dst->operation = 0;
                                        return false;
                                }
        }
        dst->realize = character;
        if (log_debug("realized")) {
                log_debug("color %d", color);
                log_debug("outcome %d", dst->operation);
                log_state(dst);
        }
        assert(check_state(dst));
        cleanup(dst);
        assert(check_state(dst));
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
                assert(!feof(global_props->file));

                assert(fscanf(global_props->file, "%"SCNu32" %"SCNu32, &(global_props->num_species),
                              &(global_props->num_characters)) != EOF);
        }

        init_state(stp, global_props->num_species, global_props->num_characters);
        stp->num_species = global_props->num_species;
        stp->num_characters = global_props->num_characters;
        stp->matrix = GC_MALLOC_ATOMIC(stp->num_species * stp->num_characters * sizeof(uint32_t));
        assert(stp->matrix != NULL);
        for(uint32_t s=0; s < stp->num_species; s++)
                for(uint32_t c=0; c < stp->num_characters; c++) {
                        uint32_t x;
                        assert(fscanf(global_props->file, "%"SCNu32, &x) != EOF || s == 0 && c == 0);
                        if (feof(global_props->file)) {
                                fclose(global_props->file);
                                return false;
                        }
                        matrix_set_value(stp, s, c, x);
                }
        if (log_debug("MATRIX"))
                for(uint32_t s=0; s < stp->num_species; s++) {
                        for(uint32_t c=0; c < stp->num_characters; c++)
                                fprintf(stderr, "%d", matrix_get_value(stp, s, c));
                        fprintf(stderr, "\n");
                }

        /* red-black graph */
        for(uint32_t s=0; s < stp->num_species; s++)
                for(uint32_t c=0; c < stp->num_characters; c++)
                        if (matrix_get_value(stp, s, c) == 1)
                                graph_add_edge(stp->red_black, s, c + stp->num_species);

        /* check the red-black graph */
        for(uint32_t s=0; s < stp->num_species; s++)
                for(uint32_t c=0; c < stp->num_characters; c++)
                        assert(matrix_get_value(stp, s, c) == 0 && !graph_edge_p(stp->red_black, s, c + stp->num_species) ||
                               matrix_get_value(stp, s, c) == 1 && graph_edge_p(stp->red_black, s, c + stp->num_species));

        /* conflict graph */
        for(uint32_t c1 = 0; c1 < stp->num_characters; c1++)
                for(uint32_t c2 = c1 + 1; c2 < stp->num_characters; c2++) {
                        uint32_t states[2][2] = { {0, 0}, {0, 0} };
                        for(uint32_t s=0; s < stp->num_species; s++)
                                states[matrix_get_value(stp, s, c1)][matrix_get_value(stp, s, c2)] = 1;
                        if(states[0][0] + states[0][1] + states[1][0] + states[1][1] == 4)
                                graph_add_edge(stp->conflict, c1, c2);
                }

        assert(check_state(stp));
        if (log_debug("STATE"))
                log_state(stp);
        return true;
}

/*
  \brief Simplify the instance whenever possible.

  We remove null characters and species.
*/
void cleanup(state_s *stp) {
        if (log_debug("Cleanup"))
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


void init_state(state_s *stp, uint32_t nspecies, uint32_t nchars) {
        assert(stp != NULL);
        stp->num_characters_orig = nchars;
        stp->num_species_orig = nspecies;
        stp->num_characters = 0;
        stp->num_species = 0;
        stp->realize = 0;
        log_debug("malloc new %p %p", stp->red_black, stp->conflict);
        stp->current_states = GC_MALLOC_ATOMIC(nchars * sizeof(uint32_t));
        assert(stp->current_states != NULL);
        stp->species = GC_MALLOC_ATOMIC(nspecies * sizeof(uint32_t));
        assert(stp->species != NULL);
        stp->characters = GC_MALLOC_ATOMIC(nchars * sizeof(uint32_t));
        assert(stp->characters != NULL);
        stp->colors = GC_MALLOC_ATOMIC(nchars * sizeof(uint8_t));
        assert(stp->colors != NULL);
        log_debug("New graph %p\n", stp->red_black);
        stp->red_black = graph_new(nspecies + nchars);
        stp->conflict = graph_new(nchars);
        stp->tried_characters = GC_MALLOC_ATOMIC(nchars * sizeof(uint32_t));
        assert(stp->tried_characters != NULL);
        stp->character_queue = GC_MALLOC_ATOMIC(nchars * sizeof(uint32_t));
        assert(stp->character_queue != NULL);

        stp->operation = 0;
        for (uint32_t i=0; i < stp->num_species_orig; i++) {
                stp->species[i] = 1;
        }
        for (uint32_t i=0; i < stp->num_characters_orig; i++) {
                stp->current_states[i] = 0;
                stp->characters[i] = 1;
                stp->colors[i] = BLACK;
                stp->tried_characters[i] = -1;
                stp->character_queue[i] = -1;
        }
        stp->character_queue_size = 0;
        stp->tried_characters_size = 0;
}

bool check_state(const state_s* stp) {
        /*
          check the state only when debugging
        */
        if (!log_debug("check_state"))
                return true;
        uint32_t err = true;
        if (stp->num_species == -1 || stp->num_species > stp->num_species_orig) {
                err = false;
                log_debug("FATAL ERROR: __FUNCTION__@__FILE__: __LINE__ (%d != %d)", stp->num_species, 0);
        }
        if (stp->num_characters == -1 || stp->num_characters > stp->num_characters_orig) {
                err = false;
                log_debug("FATAL ERROR: __FUNCTION__@__FILE__: __LINE__ (%d != %d)", stp->num_characters, 0);
        }

        uint32_t count = 0;
        for (uint32_t s = 0; s < stp->num_species_orig; s++) {
                if (stp->species[s])
                        count++;
        }
        if (count != stp->num_species) {
                err = false;
                log_debug("FATAL ERROR: __FUNCTION__@__FILE__: __LINE__ (%d != %d)", stp->num_species, count);
        }

        count = 0;
        for (uint32_t c = 0; c < stp->num_characters_orig; c++) {
                if (stp->characters[c])
                        count++;
        }
        if (count != stp->num_characters) {
                err = false;
                log_debug("FATAL ERROR: __FUNCTION__@__FILE__: __LINE__ (%d != %d)", stp->num_characters, count);
        }

        count = 0;
        for (uint32_t c = 0; c < stp->num_characters_orig; c++) {
                if (stp->current_states[c] != -1)
                        count++;
        }
        if (count != stp->num_characters) {
                err = false;
                log_debug("FATAL ERROR: __FUNCTION__@__FILE__: __LINE__ (%d != %d)", stp->num_characters, count);
        }
        return err;
}

uint32_t
characters_list(state_s * stp, uint32_t *array) {
        assert(array != NULL);
        uint32_t size = 0;
        for (unsigned int color = 1; color <= MAX_COLOR; color++)
                for (uint32_t c=0; c < stp->num_characters_orig; c++)
                        if (stp->characters[c] == color)
                                array[size++] = c;
        return size;
}


void delete_species(state_s *stp, uint32_t s) {
        log_debug("Deleting species %d", s);
        assert(s < stp->num_species_orig);
        assert(stp->species[s] > 0);
        stp->species[s] = 0;
        (stp->num_species)--;
}

void delete_character(state_s *stp, uint32_t c) {
        log_debug("Deleting character %d", c);
        assert(c < stp->num_characters_orig);
        assert(stp->characters[c] > 0);
        assert(stp->current_states[c] != -1);
        stp->characters[c] = 0;
        stp->current_states[c] = -1;
        (stp->num_characters)--;
}

void
fewest_characters(state_s* stp) {
        assert(stp != NULL);
        bool** components = connected_components(stp->red_black);
        stp->character_queue_size = stp->red_black->num_vertices + 1;
        /**
           Since we need only the connected components that
           contain at least a species, it suffices to explore
           only the connected components associated to a
           species.

           We only have to count the number of characters
           contained in the component.

           The first character in \c character_queue is the one with
           the largest degree in the red-black graph, since it is the
           most likely to be realized first.
           Moreover, when the instance has no conflict, we simulate
           the standard algorithm to compute the perfect phylogeny
        */
        for (uint32_t v = 0; v < stp->num_species_orig; v++) {
                uint32_t card = 0;
                for (uint32_t w = stp->num_species_orig; w < stp->num_species_orig + stp->num_characters_orig; w++)
                        if (components[v][w])
                                card += 1;
                if (log_debug("component: %d %d", v, card)) {
                        for (uint32_t w = 0; w < stp->num_species_orig + stp->num_characters_orig; w++)
                                fprintf(stderr, "%d ", components[v][w]);
                        fprintf(stderr, "\n");
                }
                if (card > 0 && card < stp->character_queue_size) {
                        stp->character_queue_size = card;
                        uint32_t maximal_char = 0;
                        uint32_t max_degree = 0;
                        uint32_t p = 0;
                        for (uint32_t w = stp->num_species_orig; w < stp->num_species_orig + stp->num_characters_orig; w++)
                                if (components[v][w]) {
                                        if (stp->red_black->vertices[w]->degree > max_degree) {
                                                max_degree = stp->red_black->vertices[w]->degree;
                                                maximal_char = p;
                                        }
                                        stp->character_queue[p++] = w - stp->num_species_orig;
                                }
                        uint32_t temp = stp->character_queue[0];
                        stp->character_queue[0] = stp->character_queue[maximal_char];
                        stp->character_queue[maximal_char] = temp;
                }
        }
        if (log_debug("fewest_characters: %d", stp->character_queue_size)) {
                for (uint32_t p = 0; p < stp->character_queue_size; p++)
                        fprintf(stderr, "%d ", stp->character_queue[p]);
                fprintf(stderr, "\n");
        }
}
