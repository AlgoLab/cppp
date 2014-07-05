#include<stdlib.h>
#include<stdio.h>
#include "cppp.h"



FILE* file;
ofstream outsi;
ofstream outno;
ofstream risultati;

/*!
  realize a character in src, the resulting graph is dest.
  return -1 if the realization is impossible, otherwise return
  0
*/
int
realize_character(igraph *src, igraph* dest, igraph_integer_t character) {
    igraph_vector_t *neighbors, *non_neighbors;
    int igraph_neighbors(src, neighbors, character, 0);


    igraph_copy(dest, src);


}


int main(int argc, char *argv[]) {
    char* fileName;
    int perm_figlio=1;

    // check command line (main) arguments
    if (argc != 2) {
        printf("Usage: cppp MATRIX\n");
        return 0;
    } else {
        fileName = argv[1];
        // open file and check if empty
        if ((file = fopen(fileName, "r")) == NULL) {
            printf("File %s could not be opened.\n", fileName);
            return 0;
        }
        if (feof(file)) {
            printf("File %s is empty.", fileName);
            return 0;
        }
    }

    uint32_t n; // number of species
    uint32_t m; // number of characters

    fscanf(file, "%d %d", &n, &m);
    uint8_t matrix[n][m];
    uint8_t characters_left[2*m];
    uint8_t status[2*n][m]; // 0=>inactive, 1=>active, 2=>free

    uint32_t inverse_species[n+m];
    uint32_t inverse_characters[n+m];


    /*
      The vertices of the red-black graph are the species and the characters.

      We encode the n species as the integers 0..n-1 and the characters as
      the integers n..n+m-1 (each of those integers is a vertex of the red-black
      graph)

      The vertices of the red-black graph are labeled with the encoding in the
      original red-black graph.
      Example. In the input matrix we have three species 0,1,2 and two
      characters 3,4.
      After the removal of vertex 2 and characters 3, the red-black graph has
      vertices 0,1 (species) and 2(character). The label of vertex 2 is 4, since
      it corresponds to vertex 4 in the original red-black graph.
    */

    igraph red_black[2*n];
    igraph conflict[2*n];

    uint32_t level=0;
    igraph_empty_attrs(red_black[0], n+m, 1, 0);
    igraph_i_set_attribute_table
        // read the matrix
        for (i = 0; i < n; i++)
            for (j = 0; j < m; j++) {
                uint8_t x;
                fscanf(file, "%d", &x);
                matrix[i][j];

            }
    /*
      The maximum depth of a phylogeny is 2n, therefore we can have at most 2n
      red-black and conflict graphs.
    */
