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

int duplicateDelim(int delimsCount, int i, char *delimsArr, char *argv){
  for(i = 1; i < delimsCount; i++){
    for(int j = 0; j < i; j++){
      if(argv[i] == argv[j]){
        return 1;
      }
    }
  }
  return 0;
}

int irowFn(){


}

int main (int argc, char *argv[]){
  int delimsCount = 0;
  int delimPlace = 0;
  if(argc >= 3)
    delimPlace = getDelimsCount(argc, argv, &delimsCount); //where delimiter characters are in argv
  char *delimsArr[delimsCount+1];

  if(delimsCount && argv[delimPlace]){
    delimsArr[0] = argv[delimPlace][0];
    for(int i = 0; i < delimsCount; i++){
      delimsArr[i] = argv[delimPlace][i];
    }
  }
  else{
    delimsArr[0] = ' ';
    delimsCount = 1;
  }
  duplicateDelim(delimsCount, i, delimsArr, argv[delimPlace])

  for(int i = 0; argv[i]; i++){
    if(i == 0 && delimsCount <= 1)
      i = 1;
		
    else if(i == 0 && delimsCount > 1)
      i = 3;
  }

  /*char str[10000];
  fgets(str, 10000, stdin);        */
  
  for(int i = 0; i < delimsCount; i++)
  printf("%c\n", delimsArr[i]);
}
