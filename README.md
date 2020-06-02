# Sudoku Solver

The `solver.c` program implements a Sudoku solver, capable of solving
[Project Euler Problem
96](https://projecteuler.net/problem=96). Included in the `sudoku`
directory are all 50 Project Euler puzzles, and the command `make
test` solves each in turn.

## Methodology

The code represents an unsolved Sudoku puzzle as an array of cells,
each cell represented as an int bitmap of possibilities. The code
defines several rules which are applied in turn to the puzzle. Each
rule returns the number of changes made, and, on any change, the
puzzle is tested for completion. If incomplete, the application of
rules is restarted. If all rules fail to make progress, a cell of
fewest possibilities is selected and each possibility assumed in turn,
recursively calling the solver. The solver can detect when the puzzle
is in an inconsistent state, which is used to identify an incorrect
guess.

Rules defined by the solver are:

1. Given a known cell, ensure all possibilities in the row, column and
   block have that possibility removed.
2. Identify numbers that only have a single possibility in a row,
   column or block and discard all other possibilities from the
   relevant cell.
3. Identify situations where a number is only possible in a single
   block of a row or column. In such a case, remove as a possibility
   from other cells in the block.
4. Identify situations where a possibility only appears in one row or
   column of a block. In such a case, remove it as a possibility from
   the rest of the row or column.
5. In a given row, column or block, amongst the unsolved cells,
   identify *k* cells which together have *k* possibilities. In this
   case, these possibilities must be included in these cells, so
   exclude them from other cells in the row, column or block. We call
   this the *pigeonhole check* after the *pigeonhole principle*.

# Performance

An anecdotal test with all rules enabled solved all puzzles in ~0.2s,
running in a virtual machine. No effort has been made to improve
performance, either through optimising the code or the testing process
(e.g. not running each test case in a separate process).

When only the first rule was enabled, all puzzles were still solved
(due to the guess and set approach) but took four times as long.
