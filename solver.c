#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "math.h"

#define PUZZLE_SIZE 3
#define PUZZLE_CELL_COUNT (PUZZLE_SIZE * PUZZLE_SIZE * PUZZLE_SIZE * PUZZLE_SIZE)
#define ROW_SIZE (PUZZLE_SIZE * PUZZLE_SIZE)

int set_bit_count(int n) {
  int count = 0;
  for (int i = 0; i < PUZZLE_SIZE * PUZZLE_SIZE; i++)
    if ((1 << i) & n)
      count++;
  return count;
}

int is_solved(int puzzle[]) {
  for (int i = 0; i < PUZZLE_CELL_COUNT; i++)
    if (set_bit_count(puzzle[i]) != 1)
      return 0;
  return 1;
}

int *get_row_cell_indexes(int row) {
  int* ret = malloc(sizeof(int) * ROW_SIZE);
  for (int n = 0; n < ROW_SIZE; n++)
    ret[n] = row * ROW_SIZE + n;
  return ret;
}

int *get_col_cell_indexes(int col) {
  int* ret = malloc(sizeof(int) * ROW_SIZE);
  for (int n = 0; n < ROW_SIZE; n++)
    ret[n] = ROW_SIZE * n + col;
  return ret;
}

int get_block_from_cell(int cell) {
  int row = cell / ROW_SIZE;
  int col = cell % ROW_SIZE;
  int block_row = row / PUZZLE_SIZE;
  int block_col = col / PUZZLE_SIZE;
  return block_row * PUZZLE_SIZE + block_col;
}

/* Blocks are identified by [0, ROW_SIZE) */
int *get_block_cell_indexes(int block) {
  int block_row = block / PUZZLE_SIZE;
  int block_col = block % PUZZLE_SIZE;
  int* ret = malloc(sizeof(int) * ROW_SIZE);
  for (int k = 0; k < PUZZLE_SIZE; k++) {
    for (int l = 0; l < PUZZLE_SIZE; l++) {
      int current_row = block_row * PUZZLE_SIZE + k;
      int current_col = block_col * PUZZLE_SIZE + l;
      ret[k * PUZZLE_SIZE + l] = current_row * ROW_SIZE + current_col;
    }
  }
  return ret;
}

int is_possibility(int cell, int n) {
  return cell & (1 << n);
}

#define PUZZLE_ERROR -1

/* Returns number of changes made (0 or 1), or PUZZLE_ERROR if change
   led to error state. */
int remove_possibility(int *cell, int n) {
  if (is_possibility(*cell, n)) {
    *cell &= ~(1 << n);
    return *cell ? 1 : PUZZLE_ERROR;
  }
  return 0;
}

/* Remove multiple possibilities, returning 1 or 0 depending on change
   and PUZZLE_ERROR if the change leads to an inconsistent state.*/
int mask_possibilities(int *cell, int mask) {
  int old = *cell;
  *cell &= ~mask;
  if (!*cell)
    return PUZZLE_ERROR;
  return *cell != old ? 1 : 0;
}

int rule_1_check(int puzzle[], int cell, int cells[]) {
  int changes = 0;
  for (int j = 0; j < ROW_SIZE; j++) {
    int this_cell = cells[j];
    if (this_cell == cell)
      continue;
    if (set_bit_count(puzzle[this_cell]) != 1)
      continue;
    int change = mask_possibilities(&puzzle[cell], puzzle[this_cell]);
    if (change == PUZZLE_ERROR)
      return PUZZLE_ERROR;
    changes += change;
  }
  return changes;
}

/* Remove possibilities based on known cells in rows, columns and
   blocks. Returns number of changes made, or PUZZLE_ERROR if
   inconsistent state detected. */
int rule_1(int puzzle[]) {
   int changes = 0;
   for (int i = 0; i < PUZZLE_CELL_COUNT; i++) {
     if (set_bit_count(puzzle[i]) > 1) {
       /* Check along row */
       int row = i / ROW_SIZE;
       int *cells = get_row_cell_indexes(row);
       int change = rule_1_check(puzzle, i, cells);
       free(cells);
       if (change == PUZZLE_ERROR)
         return PUZZLE_ERROR;
       changes += change;

       /* Check along column */
       int col = i % ROW_SIZE;
       cells = get_col_cell_indexes(col);
       change = rule_1_check(puzzle, i, cells);
       free(cells);
       if (change == PUZZLE_ERROR)
         return PUZZLE_ERROR;
       changes += change;

       /* Check blocks */
       int block = get_block_from_cell(i);
       cells = get_block_cell_indexes(block);
       change = rule_1_check(puzzle, i, cells);
       free(cells);
       if (change == PUZZLE_ERROR)
         return PUZZLE_ERROR;
       changes += change;
     }
   }
   return changes;
}

/* Returns the number of changes made to cells in the given
   set. Returns number of changes made. */
int rule_2_check(int puzzle[], int cell_indexes[]) {
  int changes = 0;
  for (int n = 0; n < ROW_SIZE; n++) {
    int unique_index = -1;
    for (int i = 0; i < ROW_SIZE; i++) {
      if (is_possibility(puzzle[cell_indexes[i]], n)) {
        if (unique_index == -1)
          unique_index = cell_indexes[i];
        else {
          unique_index = -1;
          break;
        }
      }
    }
    if (unique_index != -1 && set_bit_count(puzzle[unique_index]) > 1) {
      puzzle[unique_index] = 1 << n;
      ++changes;
    }
  }
  return changes;
}

/*
 * Identifies and fills numbers that only have one home in a row,
 * column or block. Returns number of changes made.
 */
int rule_2(int puzzle[]) {
  int changes = 0;

  /* Check rows */
  for (int row = 0; row < ROW_SIZE; row++) {
    int *cell_indexes = get_row_cell_indexes(row);
    changes += rule_2_check(puzzle, cell_indexes);
    free(cell_indexes);
    if (changes)
      return changes;
  }

  /* Check cols */
  for (int col = 0; col < ROW_SIZE; col++) {
    int *cell_indexes = get_col_cell_indexes(col);
    changes += rule_2_check(puzzle, cell_indexes);
    free(cell_indexes);
    if (changes)
      return changes;
  }

  /* Check blocks */
  for (int block = 0; block < ROW_SIZE; block++) {
    int *cell_indexes = get_block_cell_indexes(block);
    changes += rule_2_check(puzzle, cell_indexes);
    free(cell_indexes);
    if (changes)
      return changes;
  }

  return 0;
}

/* Returns number of changes made, or PUZZLE_ERROR if an inconsistent
   state is detected. */
int rule_3_check(int puzzle[], int cells[]) {
  int changes = 0;
  for (int n = 0; n < ROW_SIZE; n++) {
    int count = 0;
    int block_index = -1;
    for (int i = 0; i < ROW_SIZE; i++) {
      if (is_possibility(puzzle[cells[i]], n)) {
        ++count;
        int this_block_index = get_block_from_cell(cells[i]);
        if (block_index == -1)
          block_index = this_block_index;
        else if (block_index != this_block_index) {
          block_index = -1;
          break;
        }
      }
    }
    if (count > 1 && block_index != -1) {
      int *block_cells = get_block_cell_indexes(block_index);
      for (int i = 0; i < ROW_SIZE; i++) {
        /* Only consider blocks outside of the current row/column */
        int exclude = 0;
        for (int j = 0; j < ROW_SIZE; j++)
          if (block_cells[i] == cells[j]) {
            exclude = 1;
            break;
          }
        if (!exclude) {
          int change = remove_possibility(&puzzle[block_cells[i]], n);
          if (change == PUZZLE_ERROR) {
            free(block_cells);
            return PUZZLE_ERROR;
          }
          changes += change;
        }
      }
      free(block_cells);
    }
  }
  return changes;
}

/*
 * If a number can only exist within one block of a row or column,
 * then exclude from the rest of the block.
 *
 * Returns number of changes made, or PUZZLE_ERROR if inconsistent
 * state is detected.
 */
int rule_3(int puzzle[]) {
  /* Check rows */
  for (int row = 0; row < ROW_SIZE; row++) {
    int *cells = get_row_cell_indexes(row);
    int changes = rule_3_check(puzzle, cells);
    free(cells);
    if (changes != 0)
      return changes;
  }

  /* Check cols */
  for (int col = 0; col < ROW_SIZE; col++) {
    int *cells = get_col_cell_indexes(col);
    int changes = rule_3_check(puzzle, cells);
    free(cells);
    if (changes != 0)
      return changes;
  }

  return 0;
}

/*
 * If, in a block, a possibility only appears in a row or column of
 * that block, exclude it from the rest of the row or column.
 *
 * Returns the number of cells modified, or PUZZLE_ERROR on detection
 * of an inconsistent state,
 */
int rule_4(int puzzle[]) {
  int changes = 0;

  for (int block = 0; block < ROW_SIZE; block++) {
    int *cells = get_block_cell_indexes(block);
    for (int n = 0; n < ROW_SIZE; n++) {
      int row = -1;
      int col = -1;
      int count = 0;
      for (int i = 0; i < ROW_SIZE; i++) {
        int cell = cells[i];
        if (is_possibility(puzzle[cell], n)) {
          count++;

          int this_row = cell / ROW_SIZE;
          if (row == -1)
            row = this_row;
          else if (row != -2 && row != this_row)
            row = -2;

          int this_col = cell % ROW_SIZE;
          if (col == -1)
            col = this_col;
          else if (col != -2 && col != this_col)
            col = -2;
        }
      }
      if (count > 1) {
        if (row >= 0) {
          /* Exclude from row outside block */
          int *row_cells = get_row_cell_indexes(row);
          for (int i = 0; i < ROW_SIZE; i++) {
            int cell = row_cells[i];
            if (get_block_from_cell(cell) != block) {
              int change = remove_possibility(&puzzle[cell], n);
              if (change == PUZZLE_ERROR) {
                free(row_cells);
                free(cells);
                return PUZZLE_ERROR;
              }
              changes += change;
            }
          }
          free(row_cells);
        }

        if (col >= 0) {
          /* Exclude from col outside block */
          int *col_cells = get_col_cell_indexes(col);
          for (int i = 0; i < ROW_SIZE; i++) {
            int cell = col_cells[i];
            if (get_block_from_cell(cell) != block) {
              int change = remove_possibility(&puzzle[cell], n);
              if (change == PUZZLE_ERROR) {
                free(col_cells);
                free(cells);
                return PUZZLE_ERROR;
              }
              changes += change;
            }
          }
          free(col_cells);
        }
      }
    }
    free(cells);

    if (changes)
      return changes;
  }

  return 0;
}

/* Returns number of changes made, or PUZZLE_ERROR if inconsistent
   state detected. */
int pigeonhole_check(int puzzle[], int cells[], int k) {
  int changes = 0;

  /* Ignore if there are not at least k cells with k or fewer
     possibilities. */
  int count = 0;
  int possible_cells[ROW_SIZE];
  for (int i = 0; i < ROW_SIZE; i++) {
    int bit_count = set_bit_count(puzzle[cells[i]]);
    if (bit_count > 1 && bit_count <= k)
      possible_cells[count++] = cells[i];
  }
  if(count < k)
    return 0;

  /* Look at each subset of size k of possible cells. If
     pigeonhole principle applies, remove those possibilities from
     all other cells. */
  int *subsets = comb(count, k);
  for (int i = 0; i < choose(count, k); i++) {
    int set_bits = 0;
    for (int j = 0; j < k; j++)
      set_bits |= puzzle[possible_cells[subsets[i * k + j]]];
    if (set_bit_count(set_bits) == k) {
      for (int j = 0; j < ROW_SIZE; j++) {
        int cell = puzzle[cells[j]];
        if ((cell | set_bits) != set_bits && (cell & set_bits)) {
          int change = mask_possibilities(&puzzle[cells[j]], set_bits);
          if (change == PUZZLE_ERROR) {
            free(subsets);
            return PUZZLE_ERROR;
          }
          changes += change;
        }
      }
    }
  }

  free(subsets);

  return changes;
}

/*
 * Pigeonhole principle. Returns number of cells modified, or
 * PUZZLE_ERROR if inconsistent state found.
 */
int pigeonhole(int puzzle[]) {
  int changes;
  for (int k = 2; k <= 1 + ROW_SIZE / 2; k++) {
    for (int row = 0; row < ROW_SIZE; row++) {
      int *cells = get_row_cell_indexes(row);
      changes = pigeonhole_check(puzzle, cells, k);
      free(cells);
      if (changes != 0)
        return changes;
    }

    for (int col = 0; col < ROW_SIZE; col++) {
      int *cells = get_col_cell_indexes(col);
      changes = pigeonhole_check(puzzle, cells, k);
      free(cells);
      if (changes != 0)
        return changes;
    }

    for (int block = 0; block < ROW_SIZE; block++) {
      int *cells = get_block_cell_indexes(block);
      changes = pigeonhole_check(puzzle, cells, k);
      free(cells);
      if (changes != 0)
        return changes;
    }
  }

  return 0;
}

/* Find index of a cell with the lowest number of possibilities
   greater than 1. PRE: puzzle has at least one unsolved cell. */
int find_min_cell(int puzzle[]) {
  int min_count = ROW_SIZE + 1;
  int min_cell = -1;
  for (int cell = 0; cell < PUZZLE_CELL_COUNT; cell++) {
    int count = set_bit_count(puzzle[cell]);
    if (count > 1 && count < min_count) {
      min_count = count;
      min_cell = cell;
    }
  }
  return min_cell;
}

#define PUZZLE_SOLVED 0
#define PUZZLE_INCOMPLETE 1

int solve(int puzzle[]) {
  int (*rules[])(int []) = {&rule_1, &rule_2, &rule_3, &rule_4, &pigeonhole};
  while (1) {
    int changes = 0;
    if (is_solved(puzzle))
      return PUZZLE_SOLVED;

    /* Apply each rule in turn. */
    for (int i = 0; i < sizeof(rules) / sizeof(typeof(rules[0])); i++) {
      changes = rules[i](puzzle);
      if (changes == PUZZLE_ERROR)
        return PUZZLE_ERROR;
      if (changes)
        break;
    }
    /* On any change, re-test for success and start again at first
       rule. */
    if (changes)
      continue;

    /* Guess the value of a cell and call 'solve' recursively. */

    /* We choose the first unsolved cell with the least number of
       possibilities. A better choice would be to use a cell in the
       largest pigeonhole set made up of only two-possibility cells,
       as this will yield the biggest change to the puzzle. */
    int min_cell = find_min_cell(puzzle);

    /* Get possibilities for chosen cell. */
    int possibilities[ROW_SIZE];
    int possibility_count = -1;
    for (int i = 0; i < ROW_SIZE; i++) {
      if (puzzle[min_cell] & (1 << i))
        possibilities[++possibility_count] = i;
    }

    /* Recursively call 'solve' assuming each possibility. */
    for (int i = 0; i < possibility_count; i++) {
      /* Copy puzzle to new memory */
      int puzzle_copy[PUZZLE_CELL_COUNT];
      memcpy(puzzle_copy, puzzle, PUZZLE_CELL_COUNT * sizeof(int));
      /* Set possibility i in min_cell */
      puzzle_copy[min_cell] = 1 << possibilities[i];
      int result = solve(puzzle_copy);
      if (result == PUZZLE_SOLVED) {
        memcpy(puzzle, puzzle_copy, PUZZLE_CELL_COUNT * sizeof(int));
        return PUZZLE_SOLVED;
      }
    }

    return PUZZLE_INCOMPLETE;
  }
}

void print_puzzle(int puzzle[]) {
  for (int i = 0; i < PUZZLE_CELL_COUNT; i++) { 
    int count_printed = 0;
    for (int j = 0; j < ROW_SIZE; j++) {
      if ((1 << j) & puzzle[i]) {
        printf("%d", j + 1);
        ++count_printed;
      }
    }
    for (; count_printed < ROW_SIZE + 1; count_printed++)
      printf(" ");
    int row = i / ROW_SIZE;
    int col = i % ROW_SIZE;
    if (col == ROW_SIZE - 1) {
      printf("\n");
      if (row % PUZZLE_SIZE == PUZZLE_SIZE - 1)
        printf("\n");
    } else {
      if (col % PUZZLE_SIZE == PUZZLE_SIZE - 1)
        printf(" | ");
    }
  }
}

int main(void) {
  int puzzle[PUZZLE_CELL_COUNT];
  for (int i = 0; i < PUZZLE_SIZE * PUZZLE_SIZE; i++) {
    size_t line_len = PUZZLE_SIZE * PUZZLE_SIZE;
    char* line = NULL;
    ssize_t read;

    read = getline(&line, &line_len, stdin);
    if (read == -1)
      exit(1);

    /* TODO: check string length */
    for (int j = 0; j < PUZZLE_SIZE * PUZZLE_SIZE; j++) {
      int n = line[j] - 48;     /* TODO: check for digit character */
      int val = 0;
      if (n == 0)
        val = 0x1FF;
      else
        val = 1 << (n - 1);

      puzzle[i * PUZZLE_SIZE * PUZZLE_SIZE + j] = val;
    }
  }

  int ret = solve(puzzle);
  if (ret == PUZZLE_SOLVED)
    printf("Puzzle solved!\n");
  else if (ret == PUZZLE_INCOMPLETE)
    printf("Can't solve puzzle!\n");
  else
    printf("Inconsistent puzzle state detected!\n");

  print_puzzle(puzzle);

  exit(ret);
}
