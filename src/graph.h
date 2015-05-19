/*
  Copyright (C) 2015 by Gianluca Della Vedova


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

    Graph Library
*/


/**
   @file graph.h
   @brief This file contains all public functions that can be useful for dealing with
   graphs
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <libgen.h>     /*  for dirname(3) */
#undef basename         /*  (snide comment about libgen.h removed) */
#include <string.h>     /*  for basename(3) (GNU version) and strcmp(3) */
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <err.h>
#include <inttypes.h>
#include <gc.h>
#include "logging.h"
#include "memory.h"
#include <omp.h>
/**
   \struct graph_s
   \brief a graph is made of the adjacency list and the degree of each
   vertex, as well as the number of vertices
*/

typedef struct graph_s {
        uint32_t *degrees;
        bool *adjacency;
        uint32_t *adjacency_lists;
        bool *dirty_lists;
        bool dirty;
        uint32_t num_vertices;
} graph_s;

/**
   \brief managing graphs:
   \c graph_new creates a new graph, allocating the necessary memory
   to store the desired number of vertices.

   \c graph_add_edge adds an edge (returning false if the two vertices
   are already adjacent)

   \c graph_del_edge removes an edge (returning false if the two vertices
   are already adjacent)
*/
graph_s*
graph_new(uint32_t num_vertices);

void
graph_add_edge(graph_s* gp, uint32_t v1, uint32_t v2);

void
graph_add_edge_unsafe(graph_s* gp, uint32_t v1, uint32_t v2);

void
graph_del_edge(graph_s* gp, uint32_t v1, uint32_t v2);

void
graph_del_edge_unsafe(graph_s* gp, uint32_t v1, uint32_t v2);

bool
graph_get_edge(graph_s* gp, uint32_t v1, uint32_t v2);

uint32_t
graph_get_edge_pos(graph_s* gp, uint32_t v1, uint32_t pos);

/**
   \brief removes all edges of a graph
*/
void
graph_nuke_edges(graph_s* gp);

/**
   Updates the correct \c adjacency_lists of all dirty vertices of a
   graph
*/
void
graph_fix_edges(graph_s* gp, uint32_t v1);

/**
   Updates the correct \c adjacency_lists of a single dirty vertex of a
   graph
*/
void
graph_fix_graph(graph_s* gp);
/**
   \brief stores in \c reached all vertices that are in same connected
   component of \c gp as \c v

   It assumes that \c reached is an array of booleans that can store a
   bit for each vertex. The i-th bit will be set iff the i-th vertex
   is reachable from v
*/
void
graph_reachable(graph_s* gp, uint32_t v, bool* reached);

void
graph_pp(graph_s* gp);

void
graph_copy(graph_s* dst, graph_s* src);

/**
   \brief computes the degree of vertex \c v
*/
uint32_t
graph_degree(graph_s* gp, uint32_t v);

/**
   \brief computes the connected components of a graph

   \return a pointer to the array where each vertex has a number
   encoding the connected component it belongs to.
*/

uint32_t *
connected_components(graph_s* gp);

/**
   \brief test if a vertex is adjacent to all vertices that it can
   reach, ie it is the central vertex of a star and no other vertex is
   at distance larger than 1.
*/


bool
small_world_1(graph_s* gp, uint32_t v);


/**
   \brief check if two graphs are the same.
   return a nonzero code if they differ
*/
uint32_t graph_cmp(const graph_s *gp1, const graph_s *gp2);



// TODO
// Test if a bitmap really helps or not
// Test if a adjacency list is better
