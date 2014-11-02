parsing:mpc.c mpc.h parsing.c
	gcc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing
debug:mpc.c mpc.h parsing.c
	gcc -g -std=c99 -Wall parsing.c mpc.c -ledit -lm -o debugparsing
string:mpc.c mpc.h string.c
	gcc -std=c99 -Wall string.c mpc.c -ledit -lm -o string
