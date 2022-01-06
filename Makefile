compare: compare.c
	gcc -g -std=c99 -Wall -fsanitize=address -o compare compare.c -lm
