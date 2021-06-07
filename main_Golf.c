#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


int getDelimsCount(int argc, char *argv[], int *delimsCount){
  for(int i = 1; i < (argc - 1); i++){
    if(strstr(argv[i], "-d")){
      *delimsCount = strlen(argv[i + 1]);
      return (i + 1);
    }
  }
  return 0;
}

int duplicateDelim(int delimsCount, int i, int j, char *delimsArr, char *argv[], int delimPlace){
  for(int l = 0; l <= (i - j - 1); l++){
    if(delimsArr[l] == argv[delimPlace][i]){
      return 1;
    }
  }
  return 0;
}

int main (int argc, char *argv[]){
  int delimsCount = 0;
  int delimPlace = 0;
  if(argc >= 3)
    delimPlace = getDelimsCount(argc, argv, &delimsCount); //where delimiter characters are in argv
  char delimsArr[delimsCount+1];

  if(delimsCount && argv[delimPlace]){
    int duplicates = 0;
    for(int i = 0; i < delimsCount; i++){
      if(i > 0 && duplicateDelim(delimsCount, i, duplicates, delimsArr, argv/*[delimPlace]*/, delimPlace)){
        duplicates++;
      }
      else{
        delimsArr[i - duplicates] = argv[delimPlace][i];
      }
    }
    delimsCount = delimsCount - duplicates;
  }
  else{
    delimsArr[0] = ' ';
    delimsCount = 1;
  }

  for(int i = 0; argv[i]; i++){
    if(i == 0 && delimsCount <= 1)
      i = 1;
		
    else if(i == 0 && delimsCount > 1)
      i = 3;
  }

  /*char str[10000];
  fgets(str, 10000, stdin);        */
}
