CC     = mpicc
CFLAGS = -O2 -fopenmp -Wall -Wextra
NP     = 8

.PHONY: clean

PATTERN := $(MAKECMDGOALS)

.DEFAULT_GOAL := _noargs

_noargs:
	@echo "Usage: make <pattern>"

$(PATTERN): search
	@mpiexec -n $(NP) --oversubscribe ./search "$(PATTERN)"

search: main.c
	$(CC) $(CFLAGS) main.c -o search

clean:
	rm -f search
