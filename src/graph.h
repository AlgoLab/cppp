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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <err.h>
#include <inttypes.h>
#include <gc.h>
#include "logging.h"

/**
   \struct graph_s graph_adjacent_s
   \brief a graph and the adjacency list of each vertex
*/
typedef struct graph_vertex_s {
        uint32_t *adjacent;
        uint32_t degree;
} graph_vertex_s;

typedef struct graph_s {
        graph_vertex_s **vertices;
        /* vertices is an array of pointers to the struct containing
           the vertex */
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

bool
graph_add_edge(graph_s* gp, uint32_t v1, uint32_t v2);

void
graph_del_edge(graph_s* gp, uint32_t v1, uint32_t v2);

bool
graph_edge_p(graph_s* gp, uint32_t v1, uint32_t v2);

/**
   \brief check if a graph is internally consistent

   \return 0 if all check have been passed, otherwise an error code larger than 0.
*/
uint32_t graph_check(const graph_s* gp);

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

   \return a pointer to the array of the  connected components found
*/

bool **
connected_components(graph_s* gp);
