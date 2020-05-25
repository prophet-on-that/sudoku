#include <stdlib.h>

int factorial(int n) {
  if (n == 0)
    return 1;
  int ret = 1;
  for (int i = 1; i <= n; i++)
    ret *= i;
  return ret;
}

int choose(int n, int k) {
  return factorial(n) / (factorial(n - k) * factorial(k));
}

static int comb_helper(int n, int k, int *arr, int k_init, int next) {
  if (k == n) {
    /* Write {0 .. n - 1} into array. */
    for (int j = 0; j < n; j++)
      arr[next * k_init + j] = j;
    return next + 1;
  }

  if (k == 1) {
    /* Write {{0} .. {n - 1}} into array. */
    for (int j = 0; j < n; j++)
      arr[k_init * (next + j)] = j;
    return next + n;
  }

  /* map ((n - 1) :) (comb (n - 1) (k - 1)) */
  int next_next = comb_helper(n - 1, k - 1, arr, k_init, next);
  for (int j = next; j < next_next; j++)
    arr[j * k_init + k - 1] = n - 1;

  /* Append comb (n - 1) k to result */
  return comb_helper(n - 1, k, arr, k_init, next_next);
}

/*
 * Return all subsets of {0 .. n - 1} of size k in a single array (of
 * size k * choose(n, k)).
 */
int *comb(int n, int k) {
  int count = choose(n, k);
  int *ret = malloc(sizeof(int) * k * count);
  comb_helper(n, k, ret, k, 0);
  return ret;
}
