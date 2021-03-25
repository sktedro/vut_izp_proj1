/*
**
** The table editing commands put in by a user run in the order that the user typed them.
** This, however doesn't apply to a table processing command, which runs first
** even if it is typed into the last argument.
  ** eg. - ./sheet arow acol - first inserts a row and then inserts a column
**
** First, the program gets delimiters and removes duplicates (if there is a delimiter). 
** Then it checks, if there are arguments for editing or processing the table.
** The program runs every editing or processing command for each single line
** that it got from stdin using fgets. Also, it changes every delimiter to main
** delimiter (the first one provided by the user, or ASCII32 (space)).
**
**
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define maxStrLen 100
#define maxRowLen 10242


void processTheTab(char *buffer, int lineNum, char processCommand[10], char processCommandArgs[5][10], 
                   char selectionCommand[10], char selectionCommandArgs[5][10]);

int getDelimsCount(int argc, char *argv[], int *delimsCount){
  for(int i = 1; i < (argc - 1); i++){
    if(strstr(argv[i], "-d")){
      *delimsCount = strlen(argv[i + 1]);
      return (i + 1);
    }
  }
  return 0;
}

void getDelims(int *delimsCount, char *argv[], char *delimsArr, int delimPlace){
  if(*delimsCount && argv[delimPlace][0]){
    int duplicates = 0;
    for(int i = 0; i < *delimsCount; i++){
      if(i > 0){
        for(int j = 0; j <= (i - duplicates - 1); j++){
          if(argv[delimPlace][i] == delimsArr[j]){
            duplicates++;
          }else{
            delimsArr[i - duplicates] = argv[delimPlace][i];
          }
        }
      }
      if(i == 0){
        delimsArr[i - duplicates] = argv[delimPlace][i];
      }
    }
    *delimsCount -=duplicates;
  }else{
    delimsArr[0] = ' ';
    *delimsCount = 1;
  }
  printf("%d: %s\n", *delimsCount, delimsArr);
}

bool isCommand(char *argv[], int j, char commandsArr[][10]){
  if(argv[j] != NULL){
    for(int i = 0; commandsArr[i][0]; i++){
      if(commandsArr[i] != NULL && strcmp(argv[j], commandsArr[i]) == 0){
        return 1;
      }
    }
  }
  return 0;
}

int main (int argc, char *argv[]){
  char editCommandsArr[][10] = {
    "irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"
  };
  char processCommandsArr[][10] = {
    "cset", "tolower", "toupper", "round", "int", "copy", "swap", "move"
  };
  char selectionCommandsArr[][10] = {
    "rows", "beginswith", "contains"
  };
  /*int editCommandsArgNum[] = {
    1, 0, 1, 2, 1, 0, 1, 2
  };*/
  int processCommandsArgNum[] = {
    2, 1, 1, 1, 1, 2, 2, 2
  };
  
  int delimsCount = 0;
  int delimPlace = 0;
  if(argc >= 3){
    delimPlace = getDelimsCount(argc, argv, &delimsCount); //Get info about where the delimiter chars are in argv
  }else if(argc <= 1){
    fprintf(stderr, "NOT ENOUGH ARGUMENTS! THE PROGRAM WILL NOW EXIT.\n");
    return 0;
  }else if(argc > 100){
    fprintf(stderr, "TOO MANY ARGUMENTS! THE PROGRAM WILL NOW EXIT.\n");
    return 0;
  }
  char delimsArr[delimsCount + 1];
  getDelims(&delimsCount, argv, delimsArr, delimPlace);
  char mainDelim = delimsArr[0];
  //strcpy(mainDelim, delimsArr[0]);
  printf("%d: %c, %s\n", delimsCount, mainDelim, delimsArr);

  int editCommandsNum = 0;
  int processCommandsNum = 0;
  int selectionCommandsNum = 0;
  for(int i = 1; i < argc; i++){
    if(i == (delimPlace - 1))
      i+=2;
    if(isCommand(argv, i, editCommandsArr))
      editCommandsNum++;
    if(isCommand(argv, i, processCommandsArr))
      processCommandsNum++;
    if(isCommand(argv, i, selectionCommandsArr))
      selectionCommandsNum++;
  }
  if(!(editCommandsNum + processCommandsNum + selectionCommandsNum)){
    fprintf(stderr, "THERE'S NO ARGUMENT INDICATING WHAT SHOULD BE DONE WITH THE TABLE! THE PROGRAM WILL NOW EXIT. \n");
    return 0;  
  }
  if(processCommandsNum > 1){
    fprintf(stderr, "ONLY ONE PROCESSING COMMAND IS ACCEPTABLE (OR NONE)! THE PROGRAM WILL NOW EXIT.\n");
    return 0;
  }
  
  char processCommand[10];
  char processCommandArgs[5][10];
  char selectionCommand[10];
  char selectionCommandArgs[3][10];
  if(processCommandsNum == 1){
    for(int i = 1; i < argc; i++){
      for(long unsigned int j = 0; j < (sizeof(processCommandsArgNum) / sizeof(int)); j++){
        if(!strcmp(argv[i], processCommandsArr[j])){
          int l = 0;
          strcpy(processCommand, argv[i]);
          for(int k = 0; k < processCommandsArgNum[j]; k++){
            if((i+k) < argc){
              strcpy(processCommandArgs[l], argv[i+k]);
              l++;
            }
          }
          if(selectionCommandsNum == 1){
            if(!strcmp(argv[i-3], selectionCommandsArr[0]) || 
               !strcmp(argv[i-3], selectionCommandsArr[1]) || 
               !strcmp(argv[i-3], selectionCommandsArr[2])){
              strcpy(selectionCommand, argv[i-3]);
              strcpy(selectionCommandArgs[0], argv[i-2]);
              strcpy(selectionCommandArgs[1], argv[i-1]);
            }else{
              fprintf(stderr, "THERE IS A PROBLEM WITH YOUR SELECTION COMMAND! THE PROGRAM WILL NOW EXIT.\n");
              return 0;
            }
          }
        }
      }
    }
  }
  char buffer[maxRowLen];
  char *token;
  int lineNum = 0;
  int colNum;
  while(fgets(buffer, maxRowLen, stdin)){
    lineNum++;
    colNum = 0;
    for(int i = 0; buffer[i]; i++){
      for(int j = 0; j < delimsCount; j++){
        if(buffer[i] == delimsArr[j])
          buffer[i] = mainDelim;
      }
    }
    token = strtok(buffer, mainDelim);
    while(token){
      colNum++;
      if(processCommandsNum == 1){
        processTheTab(token, lineNum, colNum, processCommand, processCommandArgs, selectionCommand, selectionCommandArgs);
      }
      printf("%s\n%s\n", buffer, processCommandArgs[0]);
      token = strtok(NULL, mainDelim);
    }
  }
}

void processTheTab(char *token, int lineNum, int colNum, char processCommand[10], char processCommandArgs[5][10], 
                   char selectionCommand[10], char selectionCommandArgs[5][10]){
  char *strtolptr;
  char *strstrptr;

  bool doIEdit = false;
  
  if(!selectionCommand)
    doIEdit = true;
  else if(!strcmp(selectionCommand, "rows") && lineNum >= strtol(selectionCommandArgs[0], &strtolptr, 10) && 
          lineNum <= strtol(selectionCommandArgs[1], &strtolptr, 10))
    doIEdit = true;
  else if(!strcmp(selectionCommand, "beginswith") || !strcmp(selectedCommand, "contains")){
    if(!strcmp(selectionCommand, "beginswith")){
      strstrptr = strstr(selectionCommandArgs[1], token);
      printf("%s", token);
      if(colNum == strtol(selectionCommandArgs[0], &strtolptr, 10) && strstrptr == 0){
        doIEdit = true;      
      }
    }
  }
    doIEdit = true;

  if(doIEdit){

    if(!strcmp(processCommand, "cset")){

    }else if(!strcmp(processCommand, "tolower")){
  
    }else if(!strcmp(processCommand, "toupper")){
  
    }else if(!strcmp(processCommand, "round")){
  
    }else if(!strcmp(processCommand, "int")){
  
    }else if(!strcmp(processCommand, "copy")){
  
    }else if(!strcmp(processCommand, "swap")){
  
    }else if(!strcmp(processCommand, "move")){

    }
  }
}
