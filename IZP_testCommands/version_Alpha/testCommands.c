//./testCmds projName inputTab(.txt) outputTab(.txt) <testCmds(.txt) useDels(1 or 0)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char *argv[]){
  printf("WELCOME! USE THIS TEMPLATE TO RUN THE PROGRAM:\n");
  printf("./testCmds projName inputTab(.txt) outputTab(.txt) <testCmds(.txt) useDels(1 or 0)\n");
  printf("FOR EXAMPLE:\n");
  printf("./testCmds sheet tab1.txt output.txt <testEditCmds.txt 1\n");
  if(argc < 5){
    fprintf(stderr, "NOT ENOUGH ARGUMENTS.\n");
    return 1;
  }

  char cmd[1001];
  char testCmd[201];

  char outputTab[101];
  strcpy(outputTab, "./");
  strncat(outputTab, argv[3], 100);

  char dels[101] = "_|: _";
  dels[0] = '"';
  dels[4] = '"';

  char *strtolptr;
  int useDels = strtol(argv[4], &strtolptr, 10);
  if(strtolptr[0] || (useDels != 0 && useDels != 1)){
    fprintf(stderr, "READ THE HEADER.\n");
    return 2;
  }

  freopen(outputTab, "w", stdout);
  printf("WELCOME! USE THIS TEMPLATE TO RUN THE PROGRAM:\n");
  printf("./testCmds projName inputTab(.txt) outputTab(.txt) <testCmds(.txt) useDels(1 or 0)\n");
  printf("FOR EXAMPLE:\n");
  printf("./testCmds sheet tab1.txt output.txt <testEditCmds.txt 1\n");
  fclose(stdout);

  while(fgets(testCmd, 200, stdin)){
    strcpy(cmd, "./");
    strncat(cmd, argv[1], 100);
    strncat(cmd, " ", 2);
    if(useDels){
      strncat(cmd, "-d ", 4);
      strncat(cmd, dels, 101);
      strncat(cmd, " ", 2);
    }
    strncat(cmd, "<", 2);
    strncat(cmd, argv[2], 100);
    strncat(cmd, " ", 2);
    strncat(cmd, testCmd, 201);

    freopen(outputTab, "a", stdout);
    printf("\n||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    printf("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    printf("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n\n");
    printf("Input: \t%s", cmd);
    printf("Commands: \n%s", testCmd);
    printf("------------------------------------------------------------------\n");
    fclose(stdout);
    freopen(outputTab, "a", stderr);
    int ret = system(cmd);
    fclose(stderr);
    freopen(outputTab, "a", stdout);
    if(ret == 35584 || ret == 34304)
      printf("\n!!!SEGMENTATION FAULT!!!");
    printf("------------------------------------------------------------------\n");
    printf("Returned: \t%d\n", ret);
    fclose(stdout);
    ret = system(cmd);
  }
  return 0;
}
