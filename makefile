all: sheet.c
	gcc-7 -std=c99 sheet.c -o sheet -Wpedantic -Werror -Wall -Wextra -g
