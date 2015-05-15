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
        assert(gp != NULL);
#ifdef DEBUG
        if (gp->degrees == NULL)
                res = false;
        if (gp->adjacency == NULL)
                res = false;
        uint32_t n = gp->num_vertices;
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
        gp->adjacency = GC_MALLOC(num_vertices * num_vertices * sizeof(bool));
        memset(gp->adjacency, 0, num_vertices * num_vertices * sizeof(bool));
        assert(gp->adjacency != NULL);
        gp->degrees = GC_MALLOC(num_vertices * sizeof(uint32_t *));
        assert(gp->degrees != NULL);
        memset(gp->degrees, 0, num_vertices * sizeof((gp->degrees)[0]));
        return gp;
}

bool
graph_add_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        bool ret = 1 - graph_get_edge(gp, v1, v2);
        gp->adjacency[v1 * (gp->num_vertices) + v2] = 1;
        gp->adjacency[v2 * (gp->num_vertices) + v1] = 1;
        gp->degrees[v1] += 1;
        gp->degrees[v2] += 1;
        return ret;
}

bool
graph_get_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        return (gp->adjacency[v1 * (gp->num_vertices) + v2]);
}

void
graph_del_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        gp->adjacency[v1 * (gp->num_vertices) + v2] = 0;
        gp->adjacency[v2 * (gp->num_vertices) + v1] = 0;
        gp->degrees[v1] -= 1;
        gp->degrees[v2] -= 1;
}

void
graph_nuke_edges(graph_s* gp) {
        memset(gp->adjacency, 0, gp->num_vertices * gp->num_vertices * sizeof(bool));
        memset(gp->degrees, 0, gp->num_vertices * sizeof((gp->degrees)[0]));
}

/**
   \brief check if a graph is internally consistent

   \return 0 if all check have been passed, otherwise an error code larger than 0.
*/

void
graph_reachable(graph_s* gp, uint32_t v, bool* reached) {
        assert(gp != NULL);
        assert(reached != NULL);
        log_debug("graph_reachable: graph_s=%p, v=%d, reached=%p", gp, v, reached);
        uint32_t n = gp->num_vertices;
        uint32_t border[n];
        uint32_t new_border[n];
        uint32_t border_size = 1;
        memset(reached, 0, n * sizeof(bool));
        border[0] = v;
        reached[v] = true;
        for(; border_size > 0; ) {
                uint32_t new_border_size = 0;
                for (uint32_t vx = 0; vx < border_size; vx++) {
                        uint32_t v1 = border[vx];
                        for (uint32_t v2 = 0; v2 < n; v2++)
                                if (graph_get_edge(gp, v1, v2) && !reached[v2]) {
                                        new_border[new_border_size++] = v2;
                                        reached[v2] = true;
                                }
                }
                memcpy(border, new_border, new_border_size * sizeof(new_border[0]));
                border_size = new_border_size;
        }
        log_array_bool("reached: ", reached, gp->num_vertices);
        log_debug("graph_reachable: end");
}

uint32_t *
connected_components(graph_s* gp) {
        assert(gp!=NULL);
        log_debug("connected_components");
        log_debug("connected_components: graph_s=%p", gp);
        graph_pp(gp);
        uint32_t* components = GC_MALLOC(gp->num_vertices * sizeof(uint32_t));
        memset(components, 0, gp->num_vertices * sizeof(uint32_t));
        assert(components != NULL);
        bool visited[gp->num_vertices];
        memset(visited, 0, gp->num_vertices * sizeof(bool));

        uint32_t color = 0;
        for (uint32_t v = 0; v < gp->num_vertices; color++) {
                log_debug("Reaching from %d", v);
/*
  Check if v is an isolated vertex.
  In that case we do not call graph_reachable to find its connected
*/
                if (graph_degree(gp, v) == 0) {
                        components[v] = color;
                        visited[v] = true;
                } else {
                        bool current_component[gp->num_vertices];
                        graph_reachable(gp, v, current_component);
                        for (uint32_t w = 0; w < gp->num_vertices; w++)
                                if (current_component[w]) {
                                        components[w] = color;
                                        visited[w] = true;
                                }
                }
/*
  Find the next vertex in an unexplored component
*/
                uint32_t next = gp->num_vertices;
                for (uint32_t w = v + 1; w < gp->num_vertices; w++)
                        if (!visited[w]) {
                                next = w;
                                break;
                        }
                v = next;
        }
        log_array_uint32_t("component",components, gp->num_vertices);
        log_debug("connected_components: end");
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
        log_debug("graph_copy");
        check_graph(src);
        graph_pp(src);
        dst->num_vertices = src->num_vertices;
        memcpy(dst->adjacency, src->adjacency, src->num_vertices * src->num_vertices * sizeof(bool));
        memcpy(dst->degrees, src->degrees, src->num_vertices * sizeof((src->degrees)[0]));
        assert(check_graph(dst));
        log_debug("graph_copy: end");
}

uint32_t
graph_degree(graph_s* gp, uint32_t v) {
        assert(v < gp->num_vertices);
        return (gp->degrees)[v];
}

bool
small_world_1(graph_s* gp, uint32_t v) {
        for (uint32_t w = 0; w < gp->num_vertices; w++)
                if (graph_get_edge(gp, v, w) && graph_degree(gp, w) > 1)
                        return false;
        return true;
}
