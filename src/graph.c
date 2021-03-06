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


void
graph_check(const graph_s *gp) {
        assert(gp != NULL);
        unsigned int err = 0;
#ifdef DEBUG
        if (gp->degrees == NULL)
                err = 1;
        if (gp->adjacency == NULL)
                err = 2;
        uint32_t n = gp->num_vertices;

        for (uint32_t v=0; v < n; v++)
                if (gp->degrees[v] > n)
                        err = 3;

        if (err == 0)
                for (uint32_t v=0; v < n; v++) {
                        uint32_t deg = 0;
                        for (uint32_t v2=0; v2 < n; v2++) {
                                if (graph_get_edge(gp, v, v2) != graph_get_edge(gp, v, v2))
                                        err = 4;
                                if (graph_get_edge(gp, v, v2))
                                        deg += 1;
                        }
                        if (deg != graph_degree(gp, v))
                                err = 5;
                }

        /* Check if adjacency matrix and lists are the same
         */
        if (err == 0)
                for (uint32_t v=0; v < n; v++) {
                        bool from_matrix[n];
                        bool from_list[n];
                        memset(from_list, 0, n * sizeof(bool));
                        for (uint32_t v2=0; v2 < n; v2++)
                                from_matrix[v2] = graph_get_edge(gp, v, v2);
                        for (uint32_t pos=0; pos < graph_degree(gp, v); pos++)
                                from_list[graph_get_edge_pos(gp, v, pos)] = true;
                        for (uint32_t v2=0; v2 < n; v2++)
                                if (from_matrix[v2] != from_list[v2])
                                        err = 6;
                }


#endif
        if (err > 0) {
                graph_pp(gp);
                log_debug("check_graph code: %d", err);
        }
        assert(err == 0);
}


graph_s*
graph_new(uint32_t n) {
        log_debug("graph_new (n=%d)", n);
        graph_s* gp = xmalloc(sizeof(graph_s));
        gp->num_vertices = n;
        gp->adjacency = xmalloc(n * n * sizeof(bool));
        memset(gp->adjacency, 0, n * n * sizeof(bool));
        gp->degrees = xmalloc(n * sizeof(uint32_t));
        memset(gp->degrees, 0, n * sizeof(uint32_t));
        gp->adjacency_lists = xmalloc(n * n * sizeof(uint32_t));
        memset(gp->adjacency_lists, 0, n * n * sizeof(uint32_t));

        return gp;
}



void
graph_add_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        log_debug("graph_add_edge %d %d", v1, v2);
        graph_check(gp);
        gp->adjacency[v1 * (gp->num_vertices) + v2] = true;
        gp->adjacency[v2 * (gp->num_vertices) + v1] = true;
        (gp->adjacency_lists)[v1 * (gp->num_vertices) + graph_degree(gp, v1)] = v2;
        (gp->adjacency_lists)[v2 * (gp->num_vertices) + graph_degree(gp, v2)] = v1;

        (gp->degrees)[v1] += 1;
        (gp->degrees)[v2] += 1;
        graph_check(gp);
}


bool
graph_get_edge(const graph_s* gp, uint32_t v1, uint32_t v2) {
        return (gp->adjacency[v1 * (gp->num_vertices) + v2]);
}

/**
   \brief returns the (pos+1)-th vertex that is adjacent to v1
*/
uint32_t
graph_get_edge_pos(const graph_s* gp, uint32_t v, uint32_t pos) {
        assert(pos < graph_degree(gp, v));
        return (gp->adjacency_lists)[v * (gp->num_vertices) + pos];
}

void
graph_del_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        log_debug("graph_del_edge %d %d", v1, v2);
        graph_check(gp);
        gp->adjacency[v1 * (gp->num_vertices) + v2] = false;
        gp->adjacency[v2 * (gp->num_vertices) + v1] = false;

        uint32_t pos = 0;
        while ((gp->adjacency_lists)[v1 * (gp->num_vertices) + pos] != v2)
                pos += 1;
        assert(pos < graph_degree(gp, v1));
        (gp->adjacency_lists)[v1 * (gp->num_vertices) + pos] = (gp->adjacency_lists)[v1 * (gp->num_vertices) + graph_degree(gp, v1) - 1];

        pos = 0;
        while ((gp->adjacency_lists)[v2 * (gp->num_vertices) + pos] != v1)
                pos += 1;
        assert(pos < graph_degree(gp, v2));
        (gp->adjacency_lists)[v2 * (gp->num_vertices) + pos] = (gp->adjacency_lists)[v2 * (gp->num_vertices) + graph_degree(gp ,v2) - 1];

        (gp->degrees)[v1] -= 1;
        (gp->degrees)[v2] -= 1;

        log_debug("graph_del_edge %d %d: completed", v1, v2);
        graph_pp(gp);
        graph_check(gp);
}

void
graph_nuke_edges(graph_s* gp) {
        log_debug("graph_nuke_edges");
        graph_check(gp);
        memset(gp->degrees, 0, (gp->num_vertices) * sizeof((gp->degrees)[0]));
        memset(gp->adjacency, 0, (gp->num_vertices) * (gp->num_vertices) * sizeof((gp->adjacency)[0]));
        memset(gp->adjacency_lists, 0, (gp->num_vertices) * (gp->num_vertices) * sizeof((gp->adjacency_lists)[0]));
        graph_check(gp);
}


void
graph_reachable(const graph_s* gp, uint32_t v, bool* reached) {
        assert(gp != NULL);
        assert(reached != NULL);
        graph_check(gp);
        log_debug("graph_reachable: graph_s=%p, v=%d, reached=%p", gp, v, reached);
        uint32_t n = gp->num_vertices;
        uint32_t border[n];
        uint32_t new_border[n];
        uint32_t border_size = 1;
        memset(reached, 0, n * sizeof(bool));
        border[0] = v;
        reached[v] = true;
        while (border_size > 0) {
                uint32_t new_border_size = 0;
                for (uint32_t vx = 0; vx < border_size; vx++) {
                        uint32_t v1 = border[vx];
                        for (uint32_t p = 0; p < graph_degree(gp, v1); p++) {
                                uint32_t w = graph_get_edge_pos(gp, v1, p);
                                if (!reached[w]) {
                                        new_border[new_border_size++] = w;
                                        reached[w] = true;
                                }
                        }
                }
                memcpy(border, new_border, new_border_size * sizeof(new_border[0]));
                border_size = new_border_size;
        }
        log_array_bool("reached: ", reached, gp->num_vertices);
        log_debug("graph_reachable: end");
}

void
connected_components(graph_s* gp, uint32_t* components) {
        assert(gp!=NULL);
        assert(components != NULL);
        log_debug("connected_components");
        log_debug("connected_components: graph_s=%p", gp);
        graph_check(gp);
        graph_pp(gp);
        memset(components, 0, (gp->num_vertices) * sizeof(uint32_t));
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
}


void
graph_pp(const graph_s* gp) {
#ifdef DEBUG
        assert(gp != NULL);
        log_debug("graph_pp");
        uint32_t n = gp->num_vertices;
        fprintf(stderr, "Graph %p has %d vertices\n", gp, n);

        fprintf(stderr, "Adjacency matrix\n");
        for (uint32_t v=0; v < n; v++) {
                fprintf(stderr, "Vertex %d (degree %d):", v, graph_degree(gp, v));
                for (uint32_t v2=0; v2 < n; v2++)
                        if (graph_get_edge(gp, v, v2))
                                fprintf(stderr, " %d", v2);
                fprintf(stderr, "\n");
        }

        fprintf(stderr, "Adjacency lists\n");
        for (uint32_t v=0; v < n; v++) {
                fprintf(stderr, "Vertex %d (degree %d):", v, graph_degree(gp, v));
                for (uint32_t p=0; p < graph_degree(gp, v); p++)
                        fprintf(stderr, " %d", graph_get_edge_pos(gp, v, p));
                fprintf(stderr, "\n");
        }
#endif
}


void
graph_copy(graph_s* dst, const graph_s* src) {
        assert(dst != NULL);
        log_debug("graph_copy");
        graph_check(src);
        graph_pp(src);
        dst->num_vertices = src->num_vertices;
        memcpy(dst->adjacency, src->adjacency, (src->num_vertices) * (src->num_vertices) * sizeof((src->adjacency)[0]));
        memcpy(dst->adjacency_lists, src->adjacency_lists, (src->num_vertices) * (src->num_vertices) * sizeof((src->adjacency_lists)[0]));
        memcpy(dst->degrees, src->degrees, (src->num_vertices) * sizeof((src->degrees)[0]));
        log_debug("graph_copy: copied");

        if (graph_cmp(src, dst) != 0)
                log_debug("Graphs differ: %d", graph_cmp(src, dst));
        log_debug("graph_copy: dst");
        graph_pp(dst);
        assert(graph_cmp(src, dst) == 0);
        graph_check(dst);
        log_debug("graph_copy: end");
}

uint32_t
graph_degree(const graph_s* gp, uint32_t v) {
        assert(v < gp->num_vertices);
        return (gp->degrees)[v];
}


uint32_t graph_cmp(const graph_s *gp1, const graph_s *gp2) {
        assert(gp1 != NULL);
        assert(gp2 != NULL);
        graph_check(gp1);
        graph_check(gp2);

        if (gp1->num_vertices != gp2->num_vertices)
                return 1;

        if (memcmp(gp1->degrees, gp2->degrees, (gp1->num_vertices) * sizeof((gp1->degrees)[0])))
                return 2;

        for (uint32_t v = 0; v < gp1->num_vertices; v++)
                for (uint32_t w = 0; w < (gp1->degrees)[v]; w++)
                        if (gp1->adjacency_lists[w] != gp2->adjacency_lists[w])
                                return 3;

        if (memcmp(gp1->adjacency, gp2->adjacency, (gp1->num_vertices) * (gp1->num_vertices) * sizeof((gp1->adjacency)[0])))
                return 4;

        return 0;
}
