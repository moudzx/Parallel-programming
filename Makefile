CC     = mpicc
CFLAGS = -O2 -fopenmp -Wall -Wextra
LDFLAGS = -fopenmp -lpthread
NP     = 8

.PHONY: clean

.DEFAULT_GOAL := help

help:
	@echo "Usage: make <directory_to_search> <pattern>"
	@echo "Example: make /home/user/docs .txt"

%: search
	@if [ "$@" = "clean" ] || [ "$@" = "help" ]; then \
		exit 0; \
	fi; \
	if [ -z "$(word 2,$(MAKECMDGOALS))" ]; then \
		echo "Error: Pattern not specified"; \
		echo "Usage: make <directory> <pattern>"; \
		exit 1; \
	fi; \
	mpiexec -n $(NP) ./search "$@" "$(word 2,$(MAKECMDGOALS))"

search: main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f search
