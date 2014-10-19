parsing:mpc.c mpc.h parsing.c
	gcc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing
