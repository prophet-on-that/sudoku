#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
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

/* Returns 1 if possibility removed, 0 if already removed. */
int remove_possibility(int *cell, int n) {
  if (is_possibility(*cell, n)) {
    *cell &= ~(1 << n);
    return 1;
  }
  return 0;
}

int rule_1_check(int puzzle[], int cell, int cells[]) {
  int changes = 0;
  for (int j = 0; j < ROW_SIZE; j++) {
    int this_cell = cells[j];
    if (this_cell == cell)
      continue;
    if (set_bit_count(puzzle[this_cell]) != 1)
      continue;
    int new = puzzle[cell] & ~puzzle[this_cell];
    if (puzzle[cell] != new)
      changes++;
    puzzle[cell] = new;
  }
  return changes;
}

/* Remove possibilities based on known cells in rows, columns and
   blocks. */
int rule_1(int puzzle[]) {
   int changes = 0;
   for (int i = 0; i < PUZZLE_CELL_COUNT; i++) {
     if (set_bit_count(puzzle[i]) > 1) {
       /* Check along row */
       int row = i / ROW_SIZE;
       int *cells = get_row_cell_indexes(row);
       changes += rule_1_check(puzzle, i, cells);
       free(cells);

       /* Check along column */
       int col = i % ROW_SIZE;
       cells = get_col_cell_indexes(col);
       changes += rule_1_check(puzzle, i, cells);
       free(cells);

       /* Check blocks */
       int block_row = row / PUZZLE_SIZE;
       int block_col = col / PUZZLE_SIZE;
       int block = block_row * PUZZLE_SIZE + block_col;
       cells = get_block_cell_indexes(block);
       changes += rule_1_check(puzzle, i, cells);
       free(cells);
     }
   }
   return changes;
}

/* Returns the number of changes made to cells in the given set. */
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
 * column or block.
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
        if (!exclude)
          changes += remove_possibility(&puzzle[block_cells[i]], n);
      }
      free(block_cells);
    }
  }
  return changes;
}

/*
 * If a number can only exist within one block of a row or column,
 * then exclude from the rest of the block.
 */
int rule_3(int puzzle[]) {
  int changes = 0;

  /* Check rows */
  for (int row = 0; row < ROW_SIZE; row++) {
    int *cells = get_row_cell_indexes(row);
    changes += rule_3_check(puzzle, cells);
    free(cells);
    if (changes)
      return changes;
  }

  /* Check cols */
  for (int col = 0; col < ROW_SIZE; col++) {
    int *cells = get_col_cell_indexes(col);
    changes += rule_3_check(puzzle, cells);
    free(cells);
    if (changes)
      return changes;
  }

  return 0;
}

/* If, in a block, a possibility only appears in a row or column of
   that block, exclude it from the rest of the row or column.  */
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
            if (get_block_from_cell(cell) != block)
              changes += remove_possibility(&puzzle[cell], n);
          }
          free(row_cells);
        }

        if (col >= 0) {
          /* Exclude from col outside block */
          int *col_cells = get_col_cell_indexes(col);
          for (int i = 0; i < ROW_SIZE; i++) {
            int cell = col_cells[i];
            if (get_block_from_cell(cell) != block)
              changes += remove_possibility(&puzzle[cell], n);
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
          puzzle[cells[j]] &= ~set_bits;
          changes++;
        }
      }
    }
  }

  free(subsets);

  return changes;
}

/*
 * Pigeonhole principle.
 */
int pigeonhole(int puzzle[]) {
  int changes = 0;

  for (int k = 2; k <= 1 + ROW_SIZE / 2; k++) {
    for (int row = 0; row < ROW_SIZE; row++) {
      int *cells = get_row_cell_indexes(row);
      changes += pigeonhole_check(puzzle, cells, k);
      free(cells);
      if (changes)
        return changes;
    }

    for (int col = 0; col < ROW_SIZE; col++) {
      int *cells = get_col_cell_indexes(col);
      changes += pigeonhole_check(puzzle, cells, k);
      free(cells);
      if (changes)
        return changes;
    }

    for (int block = 0; block < ROW_SIZE; block++) {
      int *cells = get_block_cell_indexes(block);
      changes += pigeonhole_check(puzzle, cells, k);
      free(cells);
      if (changes)
        return changes;
    }
  }

  return 0;
}

int solve(int puzzle[]) {
  while (1) {
    int changes = rule_1(puzzle);
    if (is_solved(puzzle)) {
      printf("We've solved the puzzle!\n");
      return 0;
    }
    if (changes == 0) {
      int rule_2_changes = rule_2(puzzle);
      if (rule_2_changes == 0) {
        int rule_3_changes = rule_3(puzzle);
        if (rule_3_changes == 0) {
          int rule_4_changes = rule_4(puzzle);
          if (rule_4_changes == 0) {
            int pigeonhole_changes = pigeonhole(puzzle);
            if (pigeonhole_changes == 0) {
              printf("Can't solve puzzle.\n");
              return 1;
            }
          }
        }
      }
    }
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

  print_puzzle(puzzle);

  exit(ret);
}
