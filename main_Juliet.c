/*
**
** The table editing commands put in by a user do not run in the order that the user typed them.
**
** First, the program gets delimiters and removes duplicates (if there is a
** delimiter set by a user). 
** Then it checks, if there are arguments for editing or processing the table.
** The program runs every editing or processing command for each single line
** that it got from stdin using fgets. Also, it changes every delimiter to main
** delimiter (the first one provided by the user, or ASCII32 (space)).
**
** Variables:
** del is a delimiter
** cmd is a command
** editCmd is an editing command
** procCmd is a processing command
** selCmd is a selection command
** ct means count
** arr means an array, of course
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define maxStrLen 100
#define maxRowLen 10242

//TODO osetrit, aby ak zadam dcol a ziaden argument za tym, vyhodilo chybu a
//nie segfault

int getDelsCt(int argc, char *argv[], int *delsCt){
  for(int i = 1; i < (argc - 1); i++){
    if(strstr(argv[i], "-d")){
      *delsCt = strlen(argv[i + 1]);
      return (i + 1);
    }
  }
  return 0;
}

void getDels(int *delsCt, char *argv[], char *delsArr, int delPlace){
  if(*delsCt && argv[delPlace][0]){
    int duplicates = 0;
    for(int i = 0; i < *delsCt; i++){
      if(i > 0){
        for(int j = 0; j <= (i - duplicates - 1); j++){
          if(argv[delPlace][i] == delsArr[j]) 
            duplicates++;
          else 
            delsArr[i - duplicates] = argv[delPlace][i];
        }
      }
      if(i == 0)
        delsArr[i - duplicates] = argv[delPlace][i];
    }
    *delsCt -= duplicates;
  }else{
    delsArr[0] = ' ';
    *delsCt = 1;
  }
}

bool isCmd(char *argv[], int j, char cmdsArr[][11]){
  if(argv[j] != NULL){
    for(int i = 0; cmdsArr[i][0]; i++){
      if(cmdsArr[i] != NULL && strcmp(argv[j], cmdsArr[i]) == 0){
        return 1;
      }
    }
  }
  return 0;
}

int procTheTab(char token[100], long unsigned int rowNum, int colNum, char procCmd[8], 
    char procCmdArgs[5][10], char selCmd[11], char selCmdArgs[3][10]){
  char *strtolptr;
  char *strtolptr2;
  bool doIEdit = false;
  if(!strlen(selCmd)){
    doIEdit = true;
  }else if(!strcmp(selCmd, "rows") && rowNum >= strtol(selCmdArgs[0], &strtolptr, 10) 
      && rowNum <= strtol(selCmdArgs[1], &strtolptr2, 10)){
    doIEdit = true;
  }else if(!strcmp(selCmd, "beginswith") && colNum == strtol(selCmdArgs[0], &strtolptr, 10) 
      && token == strstr(token, selCmdArgs[1])){
    doIEdit = true;     
  }else if(!strcmp(selCmd, "contains") && colNum == strtol(selCmdArgs[0], &strtolptr, 10) 
      && strstr(token, selCmdArgs[1])){
    doIEdit = true;     
  }
  if(strlen(selCmd) && strtolptr[0]){
    return 0;
  }
  if(doIEdit){
    int procCmdArg1 = strtol(procCmdArgs[0], &strtolptr, 10);
    if(colNum == procCmdArg1){
      if(!strcmp(procCmd, "cset")){
        strcpy(token, procCmdArgs[1]);
      }else if(!strcmp(procCmd, "tolower")){
        for(long unsigned int i = 0; i < strlen(token); i++)
          if(token[i] >= 'A' && token[i] <= 'Z')
            token[i] += 'a' - 'A';
      }else if(!strcmp(procCmd, "toupper")){
        for(long unsigned int i = 0; i < strlen(token); i++)
          if(token[i] >= 'a' && token[i] <= 'z')
            token[i] -= 'a' - 'A';
      }else if(!strcmp(procCmd, "round")){
        char *strtodptr;
        double num = 0.5 + strtod(token, &strtodptr);
        if(strtodptr[0]){
          return 0;
        }
        int intNum = num;
        sprintf(token, "%d", intNum);
      }else if(!strcmp(procCmd, "int")){
        int num = strtol(token, &strtolptr, 10);
        sprintf(token, "%d", num);
      }
    }
    if(strtolptr[0])
      return 0;
  }
  return 1;
}

void icolAcol(char *token, long int colNum, int prevColNum, 
    int argc, char *argv[], int delPlace, char mainDel[]){
  char *strtolptr;
  for(int i = 1; i < argc; i++){
    if(i != delPlace){
      if(!strcmp(argv[i], "icol")){
        if(colNum == strtol(argv[i+1], &strtolptr, 10)){
          char tempDel[2];
          strcpy(tempDel, mainDel);
          strncat(tempDel, token, 1);
          strcpy(token, tempDel);
        }
      }else if(!strcmp(argv[i], "acol")){
        if(colNum == prevColNum)
          strncat(token, mainDel, 1);
      }
    }
  }
}

void irow(int argc, char *argv[], int delPlace, char mainDel[], int rowNum, int colNum){
  for(int i = 1; i < argc; i++){
    char *strtolptr;
    if(i != delPlace && !strcmp(argv[i], "irow") && rowNum == (strtol(argv[i+1], &strtolptr, 10))){
      for(int j = 0; j <= colNum; j++)
        printf("%c", mainDel[0]);
      printf("\n");
    }
  }
}

int drows(int argc, char *argv[], int delPlace, int rowNum){
  char *strtolptr;
  for(int i = 1; i < argc; i++){
    if(i != delPlace){
      if(!strcmp(argv[i], "drow") && rowNum == strtol(argv[i+1], &strtolptr, 10))
          return 1;
      else if(!strcmp(argv[i], "drows"))
        if(rowNum >= strtol(argv[i+1], &strtolptr, 10) && rowNum <= strtol(argv[i+2], &strtolptr, 10))
          return 1;
    }
  }
  return 0;
}

int dcols(int argc, char *argv[], int colNum, int delPlace){
  char *strtolptr;
  for(int i = 0; i < argc; i++){
    if(i != delPlace){
      if(!strcmp(argv[i], "dcol") && colNum == strtol(argv[i+1], &strtolptr, 10))
          return strtol(argv[i+1], &strtolptr, 10);
      else if(!strcmp(argv[i], "dcols"))
        if(colNum >= strtol(argv[i+1], &strtolptr, 10) && colNum <= strtol(argv[i+2], &strtolptr, 10))
          return 1;
    }
  }
  return 0;
}

void arow(int argc, char *argv[], int delPlace, char mainDel[], int colNum){
  for(int i = 1; i < argc; i++){
    if(i != delPlace && !strcmp(argv[i], "arow")){
      for(int j = 0; j <= colNum; j++)
        printf("%c", mainDel[0]);
      printf("\n");
    }
  }
}

int copy(int argc, char *argv[], int delPlace, char *token, char *tempToken, bool *isTempTokenEmpty, long unsigned int colNum){
  for(int i = 1; i < (argc - 2); i++){
    char *strtolptr10;
    char *strtolptr11;
    if(!strcmp(argv[i], "copy")){
      long unsigned int arg1;
      arg1 = strtol(argv[i+1], &strtolptr10, 10);
      long unsigned int arg2;
      arg2 = strtol(argv[i+2], &strtolptr11, 10);
      if(strtolptr10[0] || strtolptr11[0]){
        printf("THE ARGUMENTS FOR THE COMMAND 'COPY' ARE INVALID! THE PROGRAM WILL NOW EXIT.\n");
        return 0;
      }
      if(arg1 >= arg2){
        printf("YOU CAN ONLY COPY CELL FROM LEFT TO RIGHT! THE PROGRAM WILL NOW EXIT.\n");
        return 0;
      }
      if(colNum == arg1){
        if(strlen(token)){
          strcpy(tempToken, token);
        }else{ 
          *isTempTokenEmpty = true; 
        }
      }
      if(strlen(tempToken) && colNum == arg2){ //if tempToken ain't empty and colNum is right
        if(*isTempTokenEmpty == true){
          token[0] = '\0';
        }else{
          strcpy(token, tempToken);
        }
      }
    }
    return 1;
  }
}

int main (int argc, char *argv[]){
  char editCmdsArr[][11] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"};
  char selCmdsArr[][11] =  {"rows", "beginswith", "contains"};
  char procCmdsArr[][11] = {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move"};
  int procCmdsArgNum[] =   {2, 1, 1, 1, 1, 2, 2, 2};
  long unsigned int procCmdsArgNumCt = sizeof(procCmdsArgNum) / sizeof(int);
  int delsCt = 0;
  int delPlace = 0;
  if(argc > 100){
    fprintf(stderr, "TOO MANY ARGUMENTS! THE PROGRAM WILL NOW EXIT.\n");
    return 0;
  }else if(argc >= 3)
    delPlace = getDelsCt(argc, argv, &delsCt); //Get info about where the deliter chars are in argv
  char delsArr[delsCt + 1];
  getDels(&delsCt, argv, delsArr, delPlace);
  char mainDel[2];
  mainDel[0] = delsArr[0];
  int editCmdsNum = 0;
  int procCmdsNum = 0;
  int selCmdsNum = 0;
  for(int i = 1; i < argc; i++){
    if(i == (delPlace - 1))
      i+=2; //if the program approaches -d, it skips it along with the next arg (delimiters themself)
    if(isCmd(argv, i, editCmdsArr))
      editCmdsNum++;
    if(isCmd(argv, i, procCmdsArr))
      procCmdsNum++;
    if(isCmd(argv, i, selCmdsArr))
      selCmdsNum++;
  }
  if(procCmdsNum > 1){
    fprintf(stderr, "ONLY ONE PROCESSING COMMAND IS ACCEPTABLE (OR NONE)! THE PROGRAM WILL NOW EXIT.\n");
    return 0;
  }
  else if(procCmdsNum > 0 && editCmdsNum > 0){
    fprintf(stderr, "YOU CANNOT EDIT AND PROCESS THE TABLE IN ONE EXECUTION! THE PROGRAM WILL NOW EXIT.\n");
    return 0; //In case of there being a table processing command and also a table editing command 
  }
  
  char procCmd[8]; //for the name of a processing command (longest one is toupper, which has 7 chars)
  char procCmdArgs[5][10];
  char selCmd[11]; //for the name of a selection command (longest one is beginswith, which has 10 chars)
  char selCmdArgs[3][10];
  if(procCmdsNum == 1){
    for(int i = 1; i < argc; i++){
      for(long unsigned int j = 0; j < procCmdsArgNumCt; j++){
        if(!strcmp(argv[i], procCmdsArr[j])){
          strcpy(procCmd, argv[i]);
          for(int k = 0; k < procCmdsArgNum[j]; k++){
            if((i+procCmdsArgNum[j]) <= argc){
              strcpy(procCmdArgs[k], argv[i+k+1]); //i+k is the place of the command name. i+k+1 is the first command argument
            }
          }
          if(selCmdsNum == 1){
            if(!strcmp(argv[i-3], selCmdsArr[0]) || //argv[i-3] is supposed to be a name of a selection command
               !strcmp(argv[i-3], selCmdsArr[1]) || 
               !strcmp(argv[i-3], selCmdsArr[2])){  //.. so we are making sure it really is.
              strcpy(selCmd, argv[i-3]); //and then writing the typed command into selCmd variable
              strcpy(selCmdArgs[0], argv[i-2]); //and finally, writing arguments of the command to selCmdArgs variable
              strcpy(selCmdArgs[1], argv[i-1]);
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
  char token[101];
  int rowNum = 0;
  int colNum = 0;
  int prevColNum = 1;
  while(fgets(buffer, maxRowLen, stdin)){
    int l = 0;
    char tempToken[101];
    bool isTempTokenEmpty = false;
    rowNum++;
    for(int i = 0; buffer[i]; i++){
      for(int j = 0; j < delsCt; j++){
        if(buffer[i] == delsArr[j]){
          buffer[i] = mainDel[0];
          if(colNum == 0)
            prevColNum++;
        }
      }
    }
    irow(argc, argv, delPlace, mainDel, rowNum, prevColNum);
    if(drows(argc, argv, delPlace, rowNum)){
      colNum = prevColNum;
      continue;
    }
    colNum = 0;
    bool dontPtNextDel = false;
    for(long unsigned int k = 0; k < strlen(buffer); k++){
      bool dontPtNextChar = false;
      if(buffer[k] != mainDel[0] && buffer[k] != '\n'){
        token[l] = buffer[k];
        token[l+1] = 0;
        l++;
      }else{
        colNum++;
        if(procCmdsNum == 1 && !editCmdsNum){
          if(!copy(argc, argv, delPlace, token, tempToken, &isTempTokenEmpty, colNum))
            return 0;
          if(!procTheTab(token, rowNum, colNum, procCmd, procCmdArgs, selCmd, selCmdArgs)){
            fprintf(stderr, "THE ARGUMENTS YOU PROVIDED CANNOT BE WORKED WITH! THE PROGRAM WILL NOW EXIT.\n");
            return 0; //In case of invalid arguments. e.g if an arg is supposed to be a number but is a character
          }
        }else if(editCmdsNum > 0 && !procCmdsNum && !selCmdsNum){
          icolAcol(token, colNum, prevColNum, argc, argv, delPlace, mainDel);
        }
        if(dcols(argc, argv, colNum, delPlace)){
          dontPtNextChar = true;
          if(colNum == 1)
            dontPtNextDel = true;
        }
        if(colNum && !strchr(token, '\n') && !dontPtNextChar){
          if(colNum > 1 && !dontPtNextDel)
            printf("%c", mainDel[0]);
          printf("%s", token);
          dontPtNextDel = false;
        }
        l = 0;
      }
    }
    if(prevColNum != colNum){
      fprintf(stderr, "NOT ALL ROWS HAVE THE SAME NUMBER OF COLUMNS! THE PROGRAM WILL NOW EXIT.\n");
      return 0; //if there are two or more rows with different numbers of columns
    }else
      prevColNum = colNum;
    printf("\n");
  }
  arow(argc, argv, delPlace, mainDel, colNum);
}

