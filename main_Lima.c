/*
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

#define maxStrLen 101   //max cell length is 100 characters
#define maxRowLen 10242 //max length of a row is 10KiB
#define maxCmds 101     //max number of arguments in one run of the program is 100
#define maxCmdLen 11    //length of the longest command
#define maxCmdArgsNum 5 //no command is supposed to have more than 4 arguments

int getDelsCt(int argc, char *argv[], int *delsCt){
  for(int i = 1; i < (argc - 1); i++){
    if(strstr(argv[i], "-d")){
      *delsCt = strlen(argv[i + 1]);
      return (i + 1);
    }
  }
  return 0;
}

//It might be good to remove duplicate delims. If the user typed too many of
//the same delims, executing the program would take too long, since every
//single del would be checked for every single char in the tab.
void getDels(int *delsCt, char *argv[], char *delsArr, int delPlace){
  if(*delsCt && argv[delPlace][0]){
    int duplicates = 0;
    delsArr[0] = argv[delPlace][0];
    for(int i = 1; i < *delsCt; i++){
      for(int j = 0; j <= (i - duplicates - 1); j++){
        if(argv[delPlace][i] == delsArr[j])
          duplicates++;
        else
          delsArr[i - duplicates] = argv[delPlace][i];
      }
    }
    *delsCt -= duplicates;
  }else{
    delsArr[0] = ' ';
    *delsCt = 1;
  }
}

bool isCmd(int argc, char *argv[], int delPlace, int j, int cmdsArgsNum[], char cmdsArr[][maxCmdLen], int *cmdsNum){
  char exceptions[][maxCmdLen] = {"cset", "beginswith", "contains"};//the only ones that can have a string as an argument
  int numOfExceptions = 3;
  if(argv[j]){
    if(j == argc)
      return true;
    for(int i = 0; cmdsArr[i][0]; i++){
      if(cmdsArr[i] && !strcmp(argv[j], cmdsArr[i])){
        int k = 0;
        if((j + k + cmdsArgsNum[i]) >= argc 
            || (delPlace > j && delPlace <= (j + cmdsArgsNum[i] + 1))){
          fprintf(stderr, "THERE ARE NOT ENOUGH ARGUMENTS FOR YOUR COMMAND '%s'! THE PROGRAM WILL NOW EXIT.\n", cmdsArr[i]);
          return false;
        }
        for(int l = 0; l < cmdsArgsNum[i]; l++){
          char *strtolptr;
          strtol(argv[j + l + 1], &strtolptr, 10);
          if(strtolptr[0] && strtolptr[0] != '-'){
            fprintf(stderr, "ARGUMENT/ARGUMENTS FOLLOWING YOUR COMMAND '%s' IS/ARE SUPPOSED TO BE A NUMBER! THE PROGRAM WILL NOW EXIT.\n", cmdsArr[i]);
            return false;
          }
          for(int m = 0; m < numOfExceptions; m++)
            if(!strcmp(cmdsArr[i], exceptions[m]))
              l = cmdsArgsNum[i];
        }
        *cmdsNum += 1;
        return true;
      }
    }
  }
  return true;
}

bool cpSpMvFn(char buffer[maxRowLen], char token[maxStrLen], char mainDel[], int procCmdArg2, bool move, bool reverseArgs){
  int actCol = 1;
  for(int i = 0; i < maxRowLen; i++){
    if(buffer[i] == mainDel[0])
      actCol++;
    if(actCol == procCmdArg2){
      i++;
      int j = 0;
      if(buffer[i+j] == mainDel[0]){
        token[0] = '\0';
        return 1;
      }  
      char theOtherToken[maxStrLen];
      while((i + j + 1) < (int)strlen(buffer) && buffer[i+j] != mainDel[0] && buffer[i+j] != '\0'){
        theOtherToken[j] = buffer[i+j];
        j++;
      }
      theOtherToken[j] = '\0';
      if(move && reverseArgs){
        strcat(theOtherToken, mainDel);
        strcat(theOtherToken, token);
        strcpy(token, theOtherToken);
        return 1;
      }
      strcpy(token, theOtherToken);
      return 1;
    }
  }
  return 0;
}

//Copy, swap and move cmds
int cpSpMv(char procCmd[maxCmdLen], int procCmdArg1, int procCmdArg2, char *token, char *tempToken, 
    int colNum, char buffer[maxRowLen], char mainDel[]){ 
  bool copy = !strcmp(procCmd, "copy");
  bool move = !strcmp(procCmd, "move");
  bool reverseArgs = procCmdArg1 > procCmdArg2;
  if(reverseArgs){
    int temp = procCmdArg1;
    procCmdArg1 = procCmdArg2;
    procCmdArg2 = temp;
  }
  if(colNum == procCmdArg1){
    strcpy(tempToken, token);
    if(copy && !reverseArgs)
      return 1;
    if(move && !reverseArgs)
      return 2;
    if(cpSpMvFn(buffer, token, mainDel, procCmdArg2, move, reverseArgs))
      return 1;
  }else{
    if(copy && reverseArgs)
      return 1;
    if(move && reverseArgs){
      token[0] = '\0';
      return 3;
    }
    if(move && !reverseArgs){
      strcat(tempToken, mainDel);
      strcat(tempToken, token);
      strcpy(token, tempToken);
    }
    strcpy(token, tempToken);
  }
  return 1;
}
//Upravit, cast dat do funkcie. Urobit z nej bool

int procTheTab(char buffer[], char mainDel[], char token[maxStrLen], char tempToken[maxStrLen], 
    int rowNum, int colNum, char procCmd[maxCmdLen], 
    char procCmdArgs[maxCmdArgsNum][maxStrLen], char selCmd[maxCmdLen], char selCmdArgs[maxCmdArgsNum][maxStrLen]){
  bool doIEdit = false;

//Do funkcie! IMPORTANT TODO
  char *strtolptr;
  strtolptr = "";
  char *strtolptr2;
  strtolptr2 = "";
  if(!strlen(selCmd)){
    doIEdit = true;
  }else if(!strcmp(selCmd, "rows")){
    if(selCmdArgs[1][0] == '-'){
      if(selCmdArgs[0][0] == '-'){ 
        if(rowNum < 0)
          doIEdit = true;
    /*    else
          doIEdit = false;*/
      }
      else if(rowNum >= strtol(selCmdArgs[0], &strtolptr, 10) 
          || (rowNum < 0 && -rowNum >= strtol(selCmdArgs[0], &strtolptr2, 10))){
        doIEdit = true;
      }
    }
    else if(1){
      if(rowNum < 0)
        rowNum = -rowNum;      
      if(rowNum >= strtol(selCmdArgs[0], &strtolptr, 10)
            && rowNum <= strtol(selCmdArgs[1], &strtolptr2, 10))
          doIEdit = true;
    }
  }else if(!strcmp(selCmd, "beginswith") && colNum == strtol(selCmdArgs[0], &strtolptr, 10) 
      && token == strstr(token, selCmdArgs[1])){
    doIEdit = true;     
  }else if(!strcmp(selCmd, "contains") && colNum == strtol(selCmdArgs[0], &strtolptr, 10) 
      && strstr(token, selCmdArgs[1])){
    doIEdit = true;     
  }
  if(strlen(selCmd) && strtolptr[0] && strtolptr[0] != '-')
    return 0;

  //char *strtolptr;
  //char *strtolptr2;
  //Odkomentovat, ked dam vrchnu cast do funkcie


  if(doIEdit){
    int procCmdArg1 = strtol(procCmdArgs[0], &strtolptr, 10);
    if(strtolptr[0])
      return 0;
    if(colNum == procCmdArg1){
      if(!strcmp(procCmd, "cset")){
        strcpy(token, procCmdArgs[1]);
      }else if(!strcmp(procCmd, "tolower")){
        for(size_t i = 0; i < strlen(token); i++)
          if(token[i] >= 'A' && token[i] <= 'Z')
            token[i] += 'a' - 'A';
      }else if(!strcmp(procCmd, "toupper")){
        for(size_t i = 0; i < strlen(token); i++)
          if(token[i] >= 'a' && token[i] <= 'z')
            token[i] -= 'a' - 'A';
      }else if(!strcmp(procCmd, "round")){
        char *strtodptr;
        double num = 0.5 + strtod(token, &strtodptr);
        if(strtodptr[0])
          return 1;
        int intNum = num;
        sprintf(token, "%d", intNum);
      }else if(!strcmp(procCmd, "int")){
        bool isNumber = true;
        for(size_t i = 0; i < strlen(token); i++)
          if((token[i] < '0' || token[i] > '9') && token[i] != '.')
            isNumber = false;
        if(isNumber)
          sprintf(token, "%d", (int)atol(token));
        return 1;
      }
    }
    if(!strcmp(procCmd, "copy") || !strcmp(procCmd, "swap") || !strcmp(procCmd, "move")){
      char *strtolptr2;
      int procCmdArg2 = strtol(procCmdArgs[1], &strtolptr2, 10);
      if(strtolptr2[0])
        return 0;
      if(procCmdArg1 != procCmdArg2)
        if(colNum == procCmdArg1 || colNum == procCmdArg2)
          return cpSpMv(procCmd, procCmdArg1, procCmdArg2, token, tempToken, colNum, buffer, mainDel); 
    }
  }
  return 1;
}

void icolAcol(char *token, long int colNum, int prevColNum, int argc, char *argv[], int delPlace, char mainDel[]){
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

bool drows(int argc, char *argv[], int delPlace, int rowNum){
  char *strtolptr;
  for(int i = 1; i < argc; i++){
    if(i != delPlace){
      if(!strcmp(argv[i], "drow") && rowNum == strtol(argv[i+1], &strtolptr, 10))
          return true;
      else if(!strcmp(argv[i], "drows"))
        if(rowNum >= strtol(argv[i+1], &strtolptr, 10) && rowNum <= strtol(argv[i+2], &strtolptr, 10))
          return true;
    }
  }
  return false;
}

bool dcols(int argc, char *argv[], int colNum, int delPlace){
  char *strtolptr;
  for(int i = 0; i < argc; i++){
    if(i != delPlace){
      if(!strcmp(argv[i], "dcol") && colNum == strtol(argv[i+1], &strtolptr, 10))
          return strtol(argv[i+1], &strtolptr, 10);
      else if(!strcmp(argv[i], "dcols"))
        if(colNum >= strtol(argv[i+1], &strtolptr, 10) && colNum <= strtol(argv[i+2], &strtolptr, 10))
          return true;
    }
  }
  return false;
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

bool getSelAndProcCmds(int argc, char *argv[], char *procCmd, char procCmdsArr[][maxCmdLen], 
    char procCmdArgs[maxCmdArgsNum][maxStrLen], int procCmdsArgNum[], size_t procCmdsArgNumCt, 
    char selCmd[maxCmdLen], char selCmdsArr[][maxCmdLen], char selCmdArgs[][maxStrLen], int selCmdsNum){
  for(int i = 1; i < argc; i++){
    for(size_t j = 0; j < procCmdsArgNumCt; j++){
      if(!strcmp(argv[i], procCmdsArr[j])){
        strcpy(procCmd, argv[i]);
        for(int k = 0; k < procCmdsArgNum[j]; k++)
          if((i+procCmdsArgNum[j]) <= argc)
            strcpy(procCmdArgs[k], argv[i+k+1]); //i+k is the place of the command name. i+k+1 is the first command argument
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
  return 1;
}

int main(int argc, char *argv[]){
  if(argc > maxCmds){
    fprintf(stderr, "TOO MANY ARGUMENTS! THE PROGRAM WILL NOW EXIT.\n");
    return 1;
  }
  int  editCmdsNum = 0;
  int  procCmdsNum = 0;

  char procCmd[maxCmdLen]; //for the name of a processing command (longest one is toupper, which has 7 chars)
  char procCmdArgs[maxCmdArgsNum][maxStrLen];
  char selCmd[maxCmdLen]; //for the name of a selection command (longest one is beginswith, which has 10 chars)
  char selCmdArgs[3][maxStrLen]; //there are two arguments for each selection command
  //procCmd a procCmdArgs viem mergnut do 1 premennej
  //to iste asi pre selCmd

  char mainDel[2];
  int delsCt = 0;
  int delPlace = getDelsCt(argc, argv, &delsCt); //Get info about where the deliter chars are in argv
  char delsArr[delsCt + 1];
  getDels(&delsCt, argv, delsArr, delPlace);
  mainDel[0] = delsArr[0];

  //Do funkcie po char prevbuffer:
  char procCmdsArr[][maxCmdLen] = {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move"};
  int  procCmdsArgNum[] = {2, 1, 1, 1, 1, 2, 2, 2};
  char selCmdsArr[][maxCmdLen] =  {"rows", "beginswith", "contains"};
  int  selCmdsNum = 0;
  size_t procCmdsArgNumCt = sizeof(procCmdsArgNum) / sizeof(int);
  for(int i = 1; i < argc; i++){
    if(i == (delPlace - 1))
      i+=2; //if the program approaches -d, it skips it along with the next arg (delimiters themself)
    int editCmdsArgNum[] = {1, 0, 1, 2, 1, 0, 1, 2};
    int selCmdsArgNum[] = {2, 2, 2};
    char editCmdsArr[][maxCmdLen] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"};
    if(!isCmd(argc, argv, delPlace, i, editCmdsArgNum, editCmdsArr, &editCmdsNum)
      || !isCmd(argc, argv, delPlace, i, procCmdsArgNum, procCmdsArr, &procCmdsNum)
      || !isCmd(argc, argv, delPlace, i, selCmdsArgNum, selCmdsArr, &selCmdsNum))
      return 2;
    if(procCmdsNum == 1){
      if(!getSelAndProcCmds(argc, argv, procCmd, procCmdsArr, procCmdArgs, procCmdsArgNum, 
          procCmdsArgNumCt, selCmd, selCmdsArr, selCmdArgs, selCmdsNum))
        return 5;
      else if(procCmdsNum && selCmdsNum)
        break;
    }
  }
  if(procCmdsNum > 1 || selCmdsNum > 1){
    fprintf(stderr, "ONLY ONE PROCESSING/SELECTION COMMAND IS ACCEPTABLE (OR NONE)! THE PROGRAM WILL NOW EXIT.\n");
    return 3;
  }
  if((procCmdsNum || selCmdsNum) && editCmdsNum){
    fprintf(stderr, "YOU CANNOT EDIT AND PROCESS THE TABLE IN ONE EXECUTION! THE PROGRAM WILL NOW EXIT.\n");
    return 4; //In case of there being a table processing command and also a table editing command 
  }



  char prevBuffer[maxRowLen]; //necessary because I don't know that a line is the last one, until I load it into a buffer
  char buffer[maxRowLen]; //lines will be temporarily saved here (one at a time) 
  char token[maxStrLen]; //generally, cells will be temporarily saved here (one at a time)
  int rowNum = 0; //keeping track of which line is the program actually processing (negative number for the last row)
  int finish = 0;
  int colNum = 0; //keeping track of which column is the program actually processing
  int prevColNum = 1; //keeps track of total columns of the previous line, which is used to check, if all the table lines have the same amount of columns
  fgets(prevBuffer, maxRowLen, stdin);//load a line into the buffer. If the line is empty, it means the previous row was the last one
  while(1){
    strcpy(buffer, prevBuffer);
    if(!fgets(prevBuffer, maxRowLen, stdin) && rowNum >= 0){ //load a line into the buffer. If the line is empty, it means the previous row was the last one
      rowNum = -rowNum; //to indicate that this is the last row (last run of the while cycle)
      finish++;
      continue;
    }
    else if(finish == 2)
      break;
    else if(finish)
      finish++;

    char tempToken[maxStrLen];
    int l = 0;
    if(rowNum >= 0)
      rowNum++;
    else
      rowNum--;
    //strcpy(prevBuffer, buffer);
    for(int i = 0; buffer[i]; i++)
      for(int j = 0; j < delsCt; j++)
        if(buffer[i] == delsArr[j]){
          buffer[i] = mainDel[0];
          if(colNum == 0)
            prevColNum++;
        }
    irow(argc, argv, delPlace, mainDel, rowNum, prevColNum);
    if(drows(argc, argv, delPlace, rowNum)){
      colNum = prevColNum;
      continue;
    }
    colNum = 0;
    bool dontPtNextDel = false;
    for(size_t k = 0; k < strlen(buffer); k++){
      //TODO vsetko vo forku do funkcie
      bool dontPtNextChar = false;
      if(buffer[k] == mainDel[0])
        if(k == 0 || buffer[k-1] == mainDel[0])
          token[l++] = 0;
      if(buffer[k] != mainDel[0] && buffer[k] != '\n'){
        token[l] = buffer[k];
        token[++l] = 0;
      }else{
        colNum++;
        if(procCmdsNum == 1){
          int procTheTabRet = procTheTab(buffer, mainDel, token, tempToken, rowNum, colNum, procCmd, procCmdArgs, selCmd, selCmdArgs);
          if(!procTheTabRet){
            fprintf(stderr, "THE ARGUMENTS YOU PROVIDED CANNOT BE WORKED WITH! THE PROGRAM WILL NOW EXIT.\n");
            return 6; //In case of invalid arguments. e.g if an arg is supposed to be a number but is a character
          }
          if(procTheTabRet >= 2)
            dontPtNextDel = true;
          if(procTheTabRet == 2)
            dontPtNextChar = true;
        }else if(editCmdsNum)
          icolAcol(token, colNum, prevColNum, argc, argv, delPlace, mainDel);
        if(dcols(argc, argv, colNum, delPlace)){
          dontPtNextChar = true;
          if(colNum == 1)
            dontPtNextDel = true;
        }
        if(colNum && !strchr(token, '\n') && !dontPtNextChar){
          if(colNum > 1 && !dontPtNextDel)
            printf("%c", mainDel[0]);
          printf("%s", token);
          token[0] = 0;
          dontPtNextDel = false;
        }
        l = 0;
      }
    }
    printf("\n");
    if(prevColNum != colNum && rowNum >= 0){
      fprintf(stderr, "NOT ALL ROWS HAVE THE SAME NUMBER OF COLUMNS! THE PROGRAM WILL NOW EXIT.\n");
      return 7; //if there are two or more rows with different numbers of columns
    }
  }
  arow(argc, argv, delPlace, mainDel, colNum);
  return 0;
}

