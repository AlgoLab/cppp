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


#include "graph.h"

static bool
check_graph(graph_s *gp) {
        bool res = true;
#ifdef DEBUG
        assert(gp != NULL);
        uint32_t n = gp->num_vertices;
        if (gp->degrees == NULL)
                res = false;
        if (gp->adjacency == NULL)
                res = false;
        for (uint32_t v=0; v < n; v++) {
                uint32_t deg = 0;
                for (uint32_t v2=0; v2 < n; v2++) {
                        if (graph_get_edge(gp, v, v2) != graph_get_edge(gp, v, v2))
                                res = false;
                        if (graph_get_edge(gp, v, v2))
                                deg += 1;
                }
                if (deg != graph_degree(gp, v))
                        res=false;
        }
#endif
        return res;
}


graph_s*
graph_new(uint32_t num_vertices) {
        graph_s* gp = GC_MALLOC(sizeof(graph_s));
        assert(gp != NULL);
        gp->num_vertices = num_vertices;
        gp->adjacency = bitmap_alloc0(num_vertices * num_vertices);
        assert(gp->adjacency != NULL);
        gp->degrees = GC_MALLOC(num_vertices * sizeof(uint32_t *));
        assert(gp->degrees != NULL);
        memset(gp->degrees, 0, num_vertices * sizeof((gp->degrees)[0]));
        return gp;
}

bool
graph_add_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        bool ret = 1 - graph_get_edge(gp, v1, v2);
        bitmap_set_bit(gp->adjacency, v1 * (gp->num_vertices) + v2);
        bitmap_set_bit(gp->adjacency, v2 * (gp->num_vertices) + v1);
        gp->degrees[v1] += 1;
        gp->degrees[v2] += 1;
        return ret;
}

bool
graph_get_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        return (bitmap_get_bit(gp->adjacency, v1 * (gp->num_vertices) + v2));
}

void
graph_del_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        bitmap_clear_bit(gp->adjacency, v1 * (gp->num_vertices) + v2);
        bitmap_clear_bit(gp->adjacency, v2 * (gp->num_vertices) + v1);
        gp->degrees[v1] -= 1;
        gp->degrees[v2] -= 1;
}

void
graph_nuke_edges(graph_s* gp) {
        bitmap_zero(gp->adjacency, gp->num_vertices * gp->num_vertices);
        memset(gp->degrees, 0, gp->num_vertices * sizeof((gp->degrees)[0]));
}

/**
   \brief check if a graph is internally consistent

   \return 0 if all check have been passed, otherwise an error code larger than 0.
*/

void
graph_reachable(graph_s* gp, uint32_t v, bitmap_word* reached) {
        assert(gp != NULL);
        assert(reached != NULL);
        log_debug("graph_reachable: %d", v);
        uint32_t n = gp->num_vertices;
        uint32_t border[n];
        uint32_t new_border[n];
        uint32_t border_size = 1;
        bitmap_zero(reached, n);
        border[0] = v;
        bitmap_set_bit(reached, v);
        for(; border_size > 0; ) {
                uint32_t new_border_size = 0;
                for (uint32_t vx = 0; vx < border_size; vx++) {
                        uint32_t v1 = border[vx];
                        for (uint32_t v2 = 0; v2 < n; v2++)
                                if (graph_get_edge(gp, v1, v2) && !bitmap_get_bit(reached, v2)) {
                                        new_border[new_border_size++] = v2;
                                        bitmap_set_bit(reached, v2);
                                }
                }
                memcpy(border, new_border, new_border_size * sizeof(new_border[0]));
                border_size = new_border_size;
        }
        log_bitmap("reached: ", reached, gp->num_vertices);
}

bitmap_word **
connected_components(graph_s* gp) {
        assert(gp!=NULL);
        log_debug("connected_components");
        bitmap_word **components = GC_MALLOC(gp->num_vertices * sizeof(bitmap_word *));
        assert(components != NULL);
        for (uint32_t i = 0; i < gp->num_vertices; i++)
                components[i] = bitmap_alloc0(gp->num_vertices);
        bitmap_word* visited = bitmap_alloc0(gp->num_vertices);

        for (uint32_t v = 0; v < gp->num_vertices;) {
                log_debug("Reaching %d", v);
/*
  Check if v is an isolated vertex.
  In that case we do not call graph_reachable to find its connected
*/
                if (graph_degree(gp, v) > 0) {
                        graph_reachable(gp, v, components[v]);
/*
  After computing a new component, it is copied to all other vertices
  of that component
*/
                        for (uint32_t w = v + 1; w < gp->num_vertices; w++)
                                if (bitmap_get_bit(components[v], w)) {
                                        bitmap_copy(components[w], components[v], gp->num_vertices);
                                        bitmap_set_bit(visited, w);
                                }
                } else
                        bitmap_set_bit(components[v], v);
                bitmap_set_bit(visited, v);
/*
  Find the next vertex in an unexplored component
*/
                uint32_t next = gp->num_vertices;
                for (uint32_t w = v + 1; w < gp->num_vertices; w++)
                        if (!bitmap_get_bit(visited, w)) {
                                next = w;
                                break;
                        }
                v = next;
        }
        return components;
}


void
graph_pp(graph_s* gp) {
#ifdef DEBUG
        assert(gp != NULL);
        check_graph(gp);
        log_debug("graph_pp");
        check_graph(gp);
        uint32_t n = gp->num_vertices;
        fprintf(stderr, "Graph %p has %d vertices\n", gp, n);
        for (uint32_t v=0; v < n; v++) {
                fprintf(stderr, "Vertex %d (degree %d):", v, graph_degree(gp, v));
                for (uint32_t v2=0; v2 < n; v2++)
                        if (graph_get_edge(gp, v, v2))
                                fprintf(stderr, " %d", v2);
                fprintf(stderr, "\n");
        }
#endif
}


void
graph_copy(graph_s* dst, graph_s* src) {
        assert(dst != NULL);
        log_debug("graph_copy: input");
        check_graph(src);
        graph_pp(src);
        dst->num_vertices = src->num_vertices;
        bitmap_copy(dst->adjacency, src->adjacency, src->num_vertices * src->num_vertices);
        memcpy(dst->degrees, src->degrees, src->num_vertices * sizeof((src->degrees)[0]));
        log_debug("graph_copy: return");
        assert(check_graph(dst));
}

uint32_t
graph_degree(graph_s* gp, uint32_t v) {
        assert(v < gp->num_vertices);
        return (gp->degrees)[v];
}
