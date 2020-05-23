#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

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

void solve(int puzzle[]) {
  while (1) {
    int changes = 0;
    for (int i = 0; i < PUZZLE_CELL_COUNT; i++) {
      if (set_bit_count(puzzle[i]) > 1) {
        /* Check along row */
        int row = i / ROW_SIZE;
        for (int j = row * ROW_SIZE; j < (row + 1) * ROW_SIZE; j++) {
          if (j == i)
            continue;
          if (set_bit_count(puzzle[j]) != 1)
            continue;
          int new = puzzle[i] & ~puzzle[j];
          if (puzzle[i] != new)
            changes++;
          puzzle[i] = new;
        }

        /* Check along column */
        int col = i % ROW_SIZE;
        for (int j = col; j < PUZZLE_CELL_COUNT; j += ROW_SIZE) {
          if (j == i)
            continue;
          if (set_bit_count(puzzle[j]) != 1)
            continue;
          int new = puzzle[i] & ~puzzle[j];
          if (puzzle[i] != new)
            changes++;
          puzzle[i] = new;
        }

        /* Check in square */
        int sq_row = row / PUZZLE_SIZE;
        int sq_col = col / PUZZLE_SIZE;
        for (int k = 0; k < PUZZLE_SIZE; k++) {
          for (int l = 0; l < PUZZLE_SIZE; l++) {
            int current_row = sq_row * PUZZLE_SIZE + k;
            int current_col = sq_col * PUZZLE_SIZE + l;
            int j = current_row * ROW_SIZE + current_col;
            if (j == i)
              continue;
            if (set_bit_count(puzzle[j]) != 1)
              continue;
            int new = puzzle[i] & ~puzzle[j];
            if (puzzle[i] != new)
              changes++;
            puzzle[i] = new;
          }
        }
      }
    }
    if (is_solved(puzzle)) {
      printf("We've solved the puzzle!\n");
      return;
    }
    if (changes == 0) {
      printf("Can't solve puzzle.\n");
      return;
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

  solve(puzzle);

  for (int i = 0; i < 81; i++) {
    for (int j = 0; j < 9; j++) {
      if ((1 << j) & puzzle[i])
        printf("%d", j + 1);
    }
    printf(" ");
    if (i % 9 == 8)
      printf("\n");
  }
}
