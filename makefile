all: main_Xray.c
	gcc-7 -std=c99 main_Xray.c -o main_Xray -Wpedantic -Werror -Wall -Wextra -g
