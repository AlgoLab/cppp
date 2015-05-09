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
        assert(gp != NULL);
        bool res = true;
        uint32_t n = gp->num_vertices;
        for (uint32_t v=0; v < n; v++) {
                if ((gp->vertices)[v] == NULL)
                        res = false;
                if ((gp->vertices)[v]->adjacent == NULL)
                        res = false;
                for (uint32_t v2=0; v2 < n; v2++)
                        if ((gp->vertices)[v]->adjacent[v2] > n)
                                res = false;
        }
        return res;
}


graph_s*
graph_new(uint32_t num_vertices) {
        graph_s* gp = GC_MALLOC(sizeof(graph_s));
        assert(gp != NULL);
        gp->num_vertices = num_vertices;
        gp->vertices = GC_MALLOC(num_vertices * sizeof(graph_vertex_s *));
        assert(gp->vertices != NULL);

        for (uint32_t v=0; v<num_vertices; v++) {
                (gp->vertices)[v] = GC_MALLOC(sizeof(graph_vertex_s));
                assert((gp->vertices)[v] != NULL);
                graph_vertex_s* vs = (gp->vertices)[v];
                vs->adjacent = GC_MALLOC(num_vertices * sizeof(uint32_t));
                assert(vs->adjacent != NULL);
                for (uint32_t v2=0; v2<num_vertices; v2++)
                        vs->adjacent[v2] = 0;
                vs->degree = 0;
        }
        return gp;
}

bool
graph_add_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        bool ret = 1 - (gp->vertices)[v1]->adjacent[v2];
        (gp->vertices)[v1]->adjacent[v2] = 1;
        (gp->vertices)[v2]->adjacent[v1] = 1;
        (gp->vertices)[v1]->degree += 1;
        (gp->vertices)[v2]->degree += 1;
        return ret;
}

bool
graph_edge_p(graph_s* gp, uint32_t v1, uint32_t v2) {
        if ((gp->vertices)[v1]->adjacent[v2] > 0)
                return true;
        else
                return false;
}

void
graph_del_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        (gp->vertices)[v1]->adjacent[v2] = 0;
        (gp->vertices)[v2]->adjacent[v1] = 0;
        (gp->vertices)[v1]->degree -= 1;
        (gp->vertices)[v2]->degree -= 1;
}
/**
   \brief check if a graph is internally consistent

   \return 0 if all check have been passed, otherwise an error code larger than 0.
*/
uint32_t graph_check(const graph_s* gp) {
        // TOOD
        return 0;
}

void
graph_reachable(graph_s* gp, uint32_t v, bool* reached) {
        assert(gp != NULL);
        assert(reached != NULL);
        log_debug("graph_reachable: %d", v);
        uint32_t n = gp->num_vertices;
        bool border[n];
        memset(reached, 0, n * sizeof(reached[0]));
        memset(border, 0, n * sizeof(border[0]));
        border[v] = true;
        for(bool exp = true; exp;) {
                exp = false;
                bool new_border[n];
                memset(new_border, 0, n * sizeof(new_border[0]));
                for (uint32_t v1 = 0; v1 < n; v1++)
                        if (border[v1])
                                for (uint32_t v2 = 0; v2 < n; v2++)
                                        if (graph_edge_p(gp, v1, v2) && !border[v2] && !reached[v2]) {
                                                exp = true;
                                                new_border[v2] = true;
                                                reached[v2] = true;
                                        }
                memcpy(border, new_border, n * sizeof(new_border[0]));
        }
        log_array("reached: ", reached, gp->num_vertices);
}

bool **
connected_components(graph_s* gp) {
        assert(gp!=NULL);
        log_debug("connected_components");
        bool** components = GC_MALLOC(gp->num_vertices * sizeof(bool *));
        bool visited[gp->num_vertices];
        memset(visited, 0, gp->num_vertices * sizeof(visited[0]));
        for (uint32_t v = 0; v < gp->num_vertices;) {
                log_debug("Reaching %d", v);
                components[v] = GC_MALLOC(gp->num_vertices * sizeof(bool));
                graph_reachable(gp, v, components[v]);
                visited[v]= true;
/*
  After computing a new component, it is copied to all other vertices
  of that component
*/
                for (uint32_t w = v + 1; w < gp->num_vertices; w++)
                        if (components[v][w]) {
                                components[w] = GC_MALLOC(gp->num_vertices * sizeof(bool));
                                memcpy(components[w], components[v], gp->num_vertices * sizeof(bool));
                                visited[w]= true;
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
        return components;
}


void
graph_pp(graph_s* gp) {
#ifdef DEBUG
        assert(gp != NULL);
        log_debug("graph_pp");
        check_graph(gp);
        uint32_t n = gp->num_vertices;
        fprintf(stderr, "Graph %p has %d vertices\n", gp, n);
        for (uint32_t v=0; v < n; v++) {
                fprintf(stderr, "Vertex %d (degree %d):", v, graph_degree(gp, v));
                for (uint32_t v2=0; v2 < n; v2++)
                        if (graph_edge_p(gp, v, v2))
                                fprintf(stderr, " %d", v2);
                fprintf(stderr, "\n");
        }
#endif
}


void
graph_copy(graph_s* dst, graph_s* src) {
        assert(dst != NULL);
        log_debug("graph_copy: input");
        assert(check_graph(src));
        graph_pp(src);
        dst->num_vertices = src->num_vertices;
        for (uint32_t v=0; v < src->num_vertices; v++) {
                (dst->vertices)[v]->degree = (src->vertices)[v]->degree;
                memcpy((dst->vertices)[v]->adjacent, (src->vertices)[v]->adjacent, src->num_vertices * sizeof(src->vertices)[v]->adjacent[0]);
        }
        log_debug("graph_copy: return");
        assert(check_graph(dst));
}

uint32_t
graph_degree(graph_s* gp, uint32_t v) {
        assert(v < gp->num_vertices);
        return (gp->vertices)[v]->degree;
}
