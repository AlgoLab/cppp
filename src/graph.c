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

static inline int intcmp (const void *pa, const void *pb) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        if (*(uint32_t *) pa < *(uint32_t *) pb)
                return -1;
        if (*(uint32_t *) pa > *(uint32_t *) pb)
                return 1;
        return 0;
#pragma GCC diagnostic pop
}

static void
insertion_sort(uint32_t* arr, uint32_t n) {
        uint32_t sorted_prefix = 1;
        // sorted_prefix is the number of initial elements that are
        // already sorted
        for (; sorted_prefix < n; sorted_prefix++)
                if (arr[sorted_prefix - 1] >= arr[sorted_prefix]) {
                        break;
                }
        if (sorted_prefix < n) {
/* the array is not already sorted */
                for (; sorted_prefix < n; sorted_prefix++) {
                        uint32_t x = arr[sorted_prefix];
                        while (sorted_prefix > 0 && arr[sorted_prefix - 1] > x) {
                                arr[sorted_prefix] = arr[sorted_prefix - 1];
                                sorted_prefix -= 1;
                        }
                        arr[sorted_prefix] = x;
                }
        }
}

graph_check(const graph_s *gp) {
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
        gp->adjacency = xmalloc(num_vertices * num_vertices * sizeof(uint32_t));
        gp->degrees = xmalloc(num_vertices * sizeof(bool));
        gp->adjacency_lists = xmalloc(num_vertices * sizeof(uint32_t));
        bool *dirty_lists = xmalloc(num_vertices * sizeof(bool));
        bool dirty = false;

        graph_nuke_edges(gp);
        return gp;
}

void
graph_add_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        log_debug("graph_add_edge %d %d", v1, v2);
        graph_add_edge_unsafe(gp, v1, v2);
        graph_fix_edges(gp, v1);
        graph_fix_edges(gp, v2);
}

void
graph_add_edge_unsafe(graph_s* gp, uint32_t v1, uint32_t v2) {
        graph_pp(gp);
        log_debug("graph_add_edge_unsafe %d %d", v1, v2);

        gp->adjacency[v1 * (gp->num_vertices) + graph_degree(gp, v1)] = v2;
        gp->adjacency[v2 * (gp->num_vertices) + graph_degree(gp, v2)] = v1;
        gp->degrees[v1] += 1;
        gp->degrees[v2] += 1;
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
        graph_check(gp);
        return (gp->adjacency_lists)[v * (gp->num_vertices) + pos];
}


void
graph_del_edge(graph_s* gp, uint32_t v1, uint32_t v2) {
        log_debug("graph_del_edge %d %d", v1, v2);
        graph_pp(gp);
        uint32_t k = v2;

        uint32_t* found = bsearch(&k, (gp->adjacency) + v1 * (gp->num_vertices), graph_degree(gp, v1), sizeof(uint32_t), intcmp);
        assert(found != NULL);
        log_debug("graph_del_edge. deleting %d", *found);
        *found = (gp->adjacency)[v1 * (gp->num_vertices) + graph_degree(gp, v1) - 1];
        (gp->adjacency)[v1 * (gp->num_vertices) + graph_degree(gp, v1) - 1] = -1;
        (gp->degrees)[v1] -= 1;

        k = v1;
        found = bsearch(&k, (gp->adjacency) + v2 * (gp->num_vertices), graph_degree(gp, v2), sizeof(uint32_t), intcmp);
        assert(found != NULL);
        log_debug("graph_del_edge. deleting %d", *found);
        *found = (gp->adjacency)[v2 * (gp->num_vertices) + graph_degree(gp, v2) - 1];
        (gp->degrees)[v2] -= 1;

        graph_fix_edges(gp, v1);
        graph_fix_edges(gp, v2);
}

void
graph_del_edge_unsafe(graph_s* gp, uint32_t v1, uint32_t v2) {
        log_debug("graph_del_edge_unsafe %d %d", v1, v2);
        graph_pp(gp);

        for (uint32_t pos = 0; pos < graph_degree(gp, v1); pos++)
                if ((gp->adjacency)[v1 * (gp->num_vertices) + pos] == v2) {
                        log_debug("graph_del_edge_unsafe: removed %d %d", v1, v2);
                        (gp->adjacency)[v1 * (gp->num_vertices) + pos] = (gp->adjacency)[v1 * (gp->num_vertices) + graph_degree(gp, v1) - 1];
                        (gp->adjacency)[v1 * (gp->num_vertices) + graph_degree(gp, v1) - 1] = -1;
                        (gp->degrees)[v1] -= 1;
                        break;
                }

        for (uint32_t pos = 0; pos < graph_degree(gp, v2); pos++)
                if ((gp->adjacency)[v2 * (gp->num_vertices) + pos] == v1) {
                        log_debug("graph_del_edge_unsafe: removed %d %d", v1, v2);
                        (gp->adjacency)[v2 * (gp->num_vertices) + pos] = (gp->adjacency)[v2 * (gp->num_vertices) + graph_degree(gp, v2) - 1];
                        (gp->adjacency)[v2 * (gp->num_vertices) + graph_degree(gp, v2) - 1] = -1;
                        (gp->degrees)[v2] -= 1;
                        break;
                }
        graph_pp(gp);
}


void
graph_fix_graph(graph_s* gp) {
        log_debug("graph_fix_graph");
        if (!gp->dirty)
                return;
        for (uint32_t v = 0; v < gp->num_vertices; v++)
                if (gp->dirty_lists[v])
                        graph_fix_edges(gp, v);
        gp->dirty = true;
}


void
graph_fix_edges(graph_s* gp, uint32_t v) {
        log_debug("graph_fix_edges %d", v);
        graph_pp(gp);
        if (!gp->dirty_lists[v])
                return;
        if (graph_degree(gp, v) > 0) {
                uint32_t pos = 0;
                for (uint32_t w = 0; w < gp->num_vertices; w++)
                        if (graph_get_edge(gp, v, w))
                                gp->adjacency_lists[v * (gp->num_vertices) + pos++] = w;
                assert(pos == graph_degree(gp, v));
        }
        gp->dirty_lists[v] = true;
        log_debug("graph_fix_edges: sorted %d", v);
        graph_pp(gp);
}


void
graph_nuke_edges(graph_s* gp) {
        memset(gp->degrees, 0, (gp->num_vertices) * sizeof((gp->degrees)[0]));
        memset(gp->dirty_lists, 0, (gp->num_vertices) * sizeof((gp->dirty_lists)[0]));
        gp->dirty = false;
        memset(gp->adjacency, 0, (gp->num_vertices) * (gp->num_vertices) * sizeof((gp->adjacency)[0]));
        memset(gp->adjacency_lists, 0, (gp->num_vertices) * (gp->num_vertices) * sizeof((gp->adjacency_lists)[0]));
}

/**
   \brief check if a graph is internally consistent

   \return 0 if all check have been passed, otherwise an error code larger than 0.
*/

void
graph_reachable(const graph_s* gp, uint32_t v, bool* reached) {
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

uint32_t *
connected_components(graph_s* gp) {
        assert(gp!=NULL);
        log_debug("connected_components");
        log_debug("connected_components: graph_s=%p", gp);
        graph_pp(gp);
        uint32_t* components = xmalloc((gp->num_vertices) * sizeof(uint32_t));
        memset(components, 0, (gp->num_vertices) * sizeof(uint32_t));
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
graph_pp(const graph_s* gp) {
#ifdef DEBUG
        assert(gp != NULL);
        log_debug("graph_pp");
        check_graph(gp);
        uint32_t n = gp->num_vertices;
        fprintf(stderr, "Graph %p has %d vertices\n", gp, n);
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
        check_graph(src);
        graph_pp(src);
        dst->num_vertices = src->num_vertices;
        memcpy(dst->adjacency, src->adjacency, (src->num_vertices) * (src->num_vertices) * sizeof((src->adjacency)[0]));
        memcpy(dst->degrees, src->degrees, (src->num_vertices) * sizeof((src->degrees)[0]));
        log_debug("graph_copy: copied");
        if (graph_cmp(src, dst) != 0)
                log_debug("Graphs differ: %d", graph_cmp(src, dst));
        log_debug("graph_copy: dst");
        graph_pp(dst);
        assert(graph_cmp(src, dst) == 0);
        assert(check_graph(dst));
        log_debug("graph_copy: end");
}

uint32_t
graph_degree(const graph_s* gp, uint32_t v) {
        assert(v < gp->num_vertices);
        return (gp->degrees)[v];
}


uint32_t graph_cmp(const graph_s *gp1, const graph_s *gp2) {
        if (gp1->num_vertices != gp2->num_vertices)
                return 1;

        if (memcmp(gp1->degrees, gp2->degrees, (gp1->num_vertices) * sizeof((gp1->degrees)[0])))
                return 2;

        for (uint32_t v = 0; v < gp1->num_vertices; v++)
                for (uint32_t w = 0; v < gp1->degrees[v]; w++)
                        if (gp1->adjacency_lists[w] != gp2->adjacency_lists[w])
                                return 3;

        if (memcmp(gp1->adjacency, gp2->adjacency, (gp1->num_vertices) * (gp1->num_vertices) * sizeof((gp1->adjacency)[0])))
                return 4;

        return 0;
}
