solver: solver.c math.c
	gcc -o $@ -g $^

.PHONY: test
test: solver
	set -e && for file in sudoku/sudoku_*; do echo $$file && ./solver < $$file; done
