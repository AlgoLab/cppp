##Program to check if a tree realizes a matrix

### check_tree

A sample call follows

```./check_tree  -m test/a.matrix -p test/a.tree```

Options:


*     ```-m``` the file containing the matrix
*     ```-p``` the file containing the phylogeny in pseudo-Newick
format
*     ```-d``` to output lots of debugging information

If the phylogeny realizes the matrix then the program produces no
output. Otherwise gives a list of species of the matrix that are not
realized by the tree.

### tree

A sample call follows

```./tree  -m test/a.matrix```

Produces the phylogeny (in pseudo-Newick format) that realizes the
matrix. The program assumes that the input matrix has a phylogeny, the
output is undefined if the matrix has no phylogeny.

Options:


*     ```-m``` the file containing the matrix
*     ```-p``` if the input is an extended matrix, alternating
positive and negative characters. This case is suitable to generate a
persistent phylogeny. If this option is missing, it computes a perfect phylogeny.
*     ```-d``` to output lots of debugging information


### generate-persistent-phylogeny

A sample call follows

```./generate-persistent-phylogeny -p ~/lib/msdir/ms -n 20 -m 30 -r 10```

Generates some random instances, all admitting a persistent phylogeny

Options:
*     ```-m``` the number of characters
*     ```-n``` the number of species
*     ```-r``` the number of instances to generate
*     ```-p``` the path to the ms executable that is used to actually
      produce the matrix.
