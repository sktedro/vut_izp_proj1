/*
** The table eding commands put in by a user do not run in the order that the user typed them.
**
** First, the program gets delimiters and removes duplicates (if there is a
** delimiter set by a user). 
** Then it checks, if there are arguments for eding or pcessing the table.
** The program runs every eding or pcessing command for each single line
** that it got from stdin using fgets. Also, it changes every delimiter to main
** delimiter (the first one provided by the user, or ASCII32 (space)).
**
** Variables:
** del is a delimiter
** cmd is a command
** edC is an eding command
** pcC is a pcessing command
** slC is a slection command
** ct means count
** arr means an array, of course
**
** S = string
** Rw = row
** Cl = column
** C = command
** As = arguments
** L = length
** N = number
** ed = eding
** pc = pcessing
** sl = slection
** plc = place
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define mxSL 101   //mx cell length is 100 characters
#define mxRwL 10242 //mx length of a row is 10KiB
#define mxC 101     //mx number of arguments in one run of the program is 100
#define mxAL 101
#define mxCL 11    //length of the longest command
#define mxCAsN 5 //no command is supposed to have more than 4 arguments

bool checkArgs(int argc, char *argv[]){
  for(int i = 0; i < argc; i++){
    int j = 0;
    while(argv[i][j]){
      if(j >= mxAL){
        fprintf(stderr, "ONE OF YOUR ARGUMENTS IS TOO LONG! THE PROGRAM WILL NOW EXIT.\n"); 
        return 0;
      }
      j++;
    }
  }
  return 1;
}

int getDelCt(int argc, char *argv[], int *delCt){
  for(int i = 1; i < (argc - 1); i++){
    if(strstr(argv[i], "-d")){
      *delCt = strlen(argv[i + 1]);
      return (i + 1);
    }
  }
  return 0;
}

//It might be good to remove duplicate delims. If the user typed too many of
//the same delims, executing the program would take too long, since every
//single del would be checked for every single char in the tab.
void getDel(int *delCt, char *argv[], char *delArr, int delPlc){
  if(*delCt && argv[delPlc][0]){
    int duplicates = 0;
    delArr[0] = argv[delPlc][0];
    for(int i = 1; i < *delCt; i++){
      for(int j = 0; j <= (i - duplicates - 1); j++){
        if(argv[delPlc][i] == delArr[j])
          duplicates++;
        else
          delArr[i - duplicates] = argv[delPlc][i];
      }
    }
    *delCt -= duplicates;
  }else{
    delArr[0] = ' ';
    *delCt = 1;
  }
}

bool isC(int argc, char *argv[], int delPlc, int j, int cmdAsN[], char cmdArr[][mxCL], int *cmdN){
  char exceptions[][mxCL] = {"cset", "beginswith", "contains"};//the only ones that can have a string as an argument
  int numOfExceptions = 3;
  if(argv[j]){
    //if(j == argc)
      //return true;
    for(int i = 0; cmdArr[i][0]; i++){
      if(/*CmdArr[i] &&*/ !strcmp(argv[j], cmdArr[i])){
        if((j + cmdAsN[i]) >= argc || (delPlc > j && delPlc <= (j + cmdAsN[i] + 1))){
          fprintf(stderr, "THERE ARE NOT ENOUGH ARGUMENTS FOR YOUR COMMAND '%s'! THE PROGRAM WILL NOW EXIT.\n", cmdArr[i]);
          return false;
        }
        for(int l = 0; l < cmdAsN[i]; l++){
          char *strtolptr;
          strtol(argv[j + l + 1], &strtolptr, 10);
          if((strtolptr[0] && strtolptr[0] != '-') 
              || (strtolptr[0] == '-' && strcmp(argv[j], "rows") && (strcmp(argv[j], "rseq") && l != 2))){  
                  //- is allowed when using rows (on both positions) or rseq (at the third place)
            fprintf(stderr, "ARGUMENT/ARGUMENTS FOLLOWING YOUR COMMAND '%s' IS/ARE SUPPOSED TO BE A NUMBER! THE PROGRAM WILL NOW EXIT.\n", cmdArr[i]);
            return false;
          }
          for(int m = 0; m < numOfExceptions; m++)
            if(!strcmp(cmdArr[i], exceptions[m]) && l == 0) //all exceptions can only have string as the second arg
              l = cmdAsN[i];
        }
        (*cmdN)++;
        return true;
      }
    }
  }
  return true;
}

bool cpSpMvFn(char buffer[mxRwL], char token[mxSL], char del[], int pcCA2, bool move, bool reverseAs){
  int actCol = 1;
  for(int i = 0; i < mxRwL; i++){
    if(buffer[i] == del[0])
      actCol++;
    if(actCol == pcCA2){
      i++;
      int j = 0;
      if(buffer[i+j] == del[0]){
        token[0] = '\0';
        return 1;
      }  
      char theOtherToken[mxSL];
      while((i + j + 1) < (int)strlen(buffer) && buffer[i+j] != del[0] && buffer[i+j] != '\0'){
        theOtherToken[j] = buffer[i+j];
        j++;
      }
      theOtherToken[j] = '\0';
      if(move && reverseAs){
        strcat(theOtherToken, del);
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

//Copy, swap and move cmd
int cpSpMv(char pcC[mxCL], int pcCA1, int pcCA2, char *token, char *tempToken, 
    int colN, char buffer[mxRwL], char del[]){ 
  int maxC = 1;
  for(int i = 0; buffer[i]; i++)
    if(buffer[i] == del[0])
      maxC++;
  bool copy = !strcmp(pcC, "copy");
  bool move = !strcmp(pcC, "move");
  bool reverseAs = pcCA1 > pcCA2;
  if(reverseAs){
    int temp = pcCA1;
    pcCA1 = pcCA2;
    pcCA2 = temp;
  }
  if(pcCA1 == 0 || maxC < pcCA2)
    return 0;
  if(colN == pcCA1){
    strcpy(tempToken, token);
    if(copy && !reverseAs)
      return 1;
    if(move && !reverseAs)
      return 2;
    if(cpSpMvFn(buffer, token, del, pcCA2, move, reverseAs))
      return 1;
  }else{
    if(copy && reverseAs)
      return 1;
    if(move && reverseAs){
      token[0] = '\0';
      return 3;
    }
    if(move && !reverseAs){
      strcat(tempToken, del);
      strcat(tempToken, token);
      strcpy(token, tempToken);
    }
    strcpy(token, tempToken);
  }
  return 1;
}

int doIEdFn(char slC[mxCL], char slCAs[mxCAsN][mxSL], char token[mxSL], int rowN, int colN){
  bool doIEd = false;
  char *strtolptr;
  int arg1 = strtol(slCAs[0], &strtolptr, 10);
  char *strtolptr2;
  int arg2 = strtol(slCAs[1], &strtolptr2, 10);
  if(!slC[0]){
    doIEd = true;
  }else if(!strcmp(slC, "rows")){
    if(slCAs[1][0] == '-'){
      if(slCAs[0][0] == '-'){ 
        if(rowN < 0)
          doIEd = true;
      }else if(rowN >= arg1 || (rowN < 0 && -rowN >= arg1))
        doIEd = true;
    }else if(arg1 > arg2){
      fprintf(stderr, "WHEN USING 'rows' SELECTION COMMAND, THE SECOND ARGUMENT HAS TO BE A BIGGER NUMBER THAN THE FIRST ONE! THE PROGRAM WILL NOW EXIT.\n");
      return 2;
    }else{
      if(rowN < 0)
        rowN = -rowN;      
      if(rowN >= arg1 && rowN <= arg2)
        doIEd = true;
    }
  }else if(!strcmp(slC, "beginswith") && colN == arg1 && token == strstr(token, slCAs[1])){
    doIEd = true;     
  }else if(!strcmp(slC, "contains") && colN == arg1 && strstr(token, slCAs[1])){
    doIEd = true;     
  }
  if(slC[0] && strtolptr[0] && strtolptr[0] != '-')
    return 2;
  if(doIEd == true)
    return 1;
  else 
    return 0;
}

int cSAMMCFn(double *cSAMMCVal, char buffer[mxRwL], char del[2], char pcC[mxCL], char pcCAs[mxCAsN][mxSL], int colN){
  char cs[][mxCL] = {"csum", "cavg", "cmin", "cmax", "ccount"}; //commands
  int csNum = 5;
  double outVal = 0;
  bool edit = false;
  bool retNULL = true;
  bool retNULLHelper = false;
  for(int i = 0; i < csNum; i++){
    if(!strcmp(pcC, cs[i])){
      edit = true;
      char *strtolptr = "";
      int arg1 = strtol(pcCAs[0], &strtolptr, 10);
      if(strtolptr[0])
        return 0;
      if(arg1 == colN){
        char *strtolptr2 = "";
        char *strtolptr3 = "";
        int arg2 = strtol(pcCAs[1], &strtolptr2, 10);
        int arg3 = strtol(pcCAs[2], &strtolptr3, 10);
        if((arg1 >= arg2 && arg1 <= arg3) || strtolptr2[0] || strtolptr3[0])
          return 0;
        if(arg2 > arg3){
          int temp = arg2;
          arg2 = arg3;
          arg3 = temp;
        }
        int actCol = 1;
        char tempTok[mxSL] = "";
        int k = 0;
        bool init = false;
        int avgCt = 0;
        for(int j = 0; j < (int)strlen(buffer); j++){
          if(buffer[j] == del[0] || buffer[j] == '\n'){
            if(actCol >= arg2 && actCol <= arg3){
              tempTok[k] = '\0';
              char *strtodptr = "";
              double tokVal = strtod(tempTok, &strtodptr);
              if(!strcmp(pcC, "csum")){
                outVal = outVal + tokVal; 
              }else if(!strcmp(pcC, "cavg")){
                outVal += tokVal;
                avgCt++;
                retNULLHelper = true;
                if(!strtodptr[0])
                  retNULL = false;
              }else if(!strcmp(pcC, "cmin") || !strcmp(pcC, "cmax")){
                retNULLHelper = true;
                if(!strtodptr[0])
                  retNULL = false;
                if(init == false && !strtodptr[0]){
                  outVal = tokVal;
                  init = true;
                }else if(!strcmp(pcC, "cmin") && outVal > tokVal){
                  outVal = tokVal;
                }else if(!strcmp(pcC, "cmax") && outVal < tokVal){
                  outVal = tokVal;
                }
              }else if(!strcmp(pcC, "ccount")){
                if(!strtodptr[0])
                  outVal++;
              }
            }
            actCol++;
            k = 0;
          }
          else if(actCol >= arg2 && actCol <= arg3){
            tempTok[k] = buffer[j]; 
            k++;
          }
        }
        if(!strcmp(pcC, "cavg"))
          outVal = outVal / avgCt;
        *cSAMMCVal = outVal;
        if(edit && retNULL && retNULLHelper)
          return 3;
        if(edit)
          return 2;
      }
    }
  }
  return 1; //if no changes have been made
}

int cseq(int colN, char pcCAs[mxCAsN][mxSL], double *cSAMMCVal){
  char *strtolptr;
  char *strtolptr2;
  char *strtolptr3;
  double arg3 = strtol(pcCAs[2], &strtolptr, 10);
  int arg1 = strtol(pcCAs[0], &strtolptr2, 10);
  int arg2 = strtol(pcCAs[1], &strtolptr3, 10);
  if(strtolptr[0] || strtolptr2[0] || strtolptr3[0])
    return 0; //error. All args have to be numbers
  if(arg1 > arg2){
    int tempVal = arg1;
    arg1 = arg2;
    arg2 = tempVal;
  }
  //TODO funkcia na switchovanie hodnot.. switch(val1, val2);
  if(colN == arg1)
    *cSAMMCVal = arg3;
  else if(colN > arg1 && colN <= arg2)
    (*cSAMMCVal)++;
  else
    return 1; //no changes
  return 4; //write the value into the table
}

int rseq(char pcCAs[mxCAsN][mxSL], int rowN, int colN, double *rSAMMCVal){
  char *strtolptr, *strtolptr2, *strtolptr3, *strtolptr4;
  int arg1 = strtol(pcCAs[0], &strtolptr, 10);
  int arg2 = strtol(pcCAs[1], &strtolptr2, 10);
  int arg3 = strtol(pcCAs[2], &strtolptr3, 10);
  int arg4 = strtol(pcCAs[3], &strtolptr4, 10);
  if(strtolptr[0] || strtolptr2[0] || (strtolptr3[0] && strtolptr3[0] != '-') || strtolptr4[0])
    return 0; //err
  if(rowN < 0)
    rowN = 0 - rowN;
  if(strtolptr3[0] == '-')
    arg3 = rowN + 1; //if there is a - as the third arg, rseq will work for every line from arg2
  if(colN == arg1 && rowN >= arg2 && rowN <= arg3){
    if(arg1 > arg2){
      int tempVal = arg1;
      arg1 = arg2;
      arg2 = tempVal;
    }
    if(rowN == arg2)
      *rSAMMCVal = arg4; 
    else if(rowN > arg2 && rowN <= arg3)
      (*rSAMMCVal)++;
    return 6; //write the value
  }
  return 1; //no changes
}

int rSAMMCFn(char token[mxSL], char pcC[mxCL], char pcCAs[mxCAsN][mxSL], int colN, int rowN, double *rSAMMCVal, int *rSAMMCAvgCt){
  char cs[][mxCL] = {"rsum", "ravg", "rmin", "rmax", "rcount"}; //commands
  if(rowN < 0)
    rowN = 0 - rowN;
  int csNum = 5;
  bool edit;
  for(int i = 0; i < csNum; i++)
    if(!strcmp(pcC, cs[i]))
      edit = true;
  if(!edit)
    return 1;
  char *strtolptr, *strtolptr2, *strtolptr3;
  int arg1 = strtol(pcCAs[0], &strtolptr, 10);
  if(strtolptr[0])
    return 0;
  if(colN == arg1){
    int arg2 = strtol(pcCAs[1], &strtolptr2, 10);
    int arg3 = strtol(pcCAs[2], &strtolptr3, 10);
    char *strtodptr = "";
    double tokVal = strtod(token, &strtodptr);
    if(rowN >= arg2 && rowN <= arg3){
      if(strtodptr[0])
        return 1;
      if(!strcmp(pcC, "rsum")){
        *rSAMMCVal += tokVal;
      }else if(!strcmp(pcC, "ravg")){
        *rSAMMCVal += tokVal;
        (*rSAMMCAvgCt)++;
      }else if(!strcmp(pcC, "rmin") || !strcmp(pcC, "rmax")){
        if(*rSAMMCAvgCt == 0){
          *rSAMMCVal = tokVal;
          (*rSAMMCAvgCt)++;
        }else if(!strcmp(pcC, "rmin") && tokVal < *rSAMMCVal)
          *rSAMMCVal = tokVal;
        else if(!strcmp(pcC, "rmax") && tokVal > *rSAMMCVal)
          *rSAMMCVal = tokVal;
      }else if(!strcmp(pcC, "rcount")){
        (*rSAMMCVal)++;
      }
    }
    if(rowN == arg3 + 1){
      if(!strcmp(pcC, "ravg"))
        *rSAMMCVal /= (*rSAMMCAvgCt);
      if(*rSAMMCAvgCt == 0 && (!strcmp(pcC, "rmin") || !strcmp(pcC, "rmax") || !strcmp(pcC, "ravg")))
        return 3;
      return 6;
    }
  }
  return 1;
}

bool pcNumsAndChars(char pcC[mxCL], char token[mxSL]){
  if(!strcmp(pcC, "tolower")){
    for(size_t i = 0; i < strlen(token); i++)
      if(token[i] >= 'A' && token[i] <= 'Z')
        token[i] += 'a' - 'A';
  }else if(!strcmp(pcC, "toupper")){
    for(size_t i = 0; i < strlen(token); i++)
      if(token[i] >= 'a' && token[i] <= 'z')
        token[i] -= 'a' - 'A';
  }else if(!strcmp(pcC, "round")){
    char *strtodptr;
    double num = 0.5 + strtod(token, &strtodptr);
    if(strtodptr[0])
      return 1;
    int intN = num;
    sprintf(token, "%d", intN);
  }else if(!strcmp(pcC, "int")){
    bool isNber = true;
    for(size_t i = 0; i < strlen(token); i++)
      if((token[i] < '0' || token[i] > '9') && token[i] != '.')
        isNber = false;
    if(isNber)
      sprintf(token, "%d", (int)atol(token));
    return 1;
  }
  return 0;
}


int pcTheTab(char buffer[], char del[], char token[mxSL], char tempToken[mxSL], 
    int rowN, int colN, char pcC[mxCL], 
    char pcCAs[mxCAsN][mxSL], char slC[mxCL], char slCAs[mxCAsN][mxSL], double *cSAMMCVal, double *rSAMMCVal, int *rSAMMCAvgCt){
  char *strtolptr;
  bool doIEd = false;
  int doIEdFnRet = doIEdFn(slC, slCAs, token, rowN, colN);
  if(doIEdFnRet == 2)
    return 0;
  else if(doIEdFnRet == 1)
    doIEd = true;
  if(doIEd){
    int cSAMMCFnRet = cSAMMCFn(cSAMMCVal, buffer, del, pcC, pcCAs, colN);
    int rSAMMCFnRet = rSAMMCFn(token, pcC, pcCAs, colN, rowN, rSAMMCVal, rSAMMCAvgCt);
    if(cSAMMCFnRet == 0 || rSAMMCFnRet == 0)
      return 0;
    else if(cSAMMCFnRet == 2)
      return 4;
    else if(cSAMMCFnRet == 3 || rSAMMCFnRet == 3)
      return 5;
    else if(rSAMMCFnRet == 6)
      return 6;
    if(!strcmp(pcC, "cseq"))
      return cseq(colN, pcCAs, cSAMMCVal);
    if(!strcmp(pcC, "rseq"))
      return rseq(pcCAs, rowN, colN, rSAMMCVal);
    int pcCA1 = strtol(pcCAs[0], &strtolptr, 10);
    if(strtolptr[0])
      return 0;
    if(colN == pcCA1){
      if(!strcmp(pcC, "cset")){
        strcpy(token, pcCAs[1]);
      }else if(pcNumsAndChars(pcC, token))
        return 1;
    }
    if(!strcmp(pcC, "copy") || !strcmp(pcC, "swap") || !strcmp(pcC, "move")){
      char *strtolptr2;
      int pcCA2 = strtol(pcCAs[1], &strtolptr2, 10);
      if(strtolptr2[0])
        return 0;
      if(pcCA1 != pcCA2)
        if(colN == pcCA1 || colN == pcCA2)
          return cpSpMv(pcC, pcCA1, pcCA2, token, tempToken, colN, buffer, del); 
    }
  }
  return 1;
}

void icolAcol(char *token, long int colN, int prevColN, int argc, char *argv[], int delPlc, char del[]){
  char *strtolptr;
  for(int i = 1; i < argc; i++){
    if(i != delPlc){
      if(!strcmp(argv[i], "icol")){
        if(colN == strtol(argv[i+1], &strtolptr, 10)){
          char tempDel[2];
          strcpy(tempDel, del);
          strncat(tempDel, token, 1);
          strcpy(token, tempDel);
        }
      }else if(!strcmp(argv[i], "acol")){
        if(colN == prevColN)
          strncat(token, del, 1);
      }
    }
  }
}

void irow(int argc, char *argv[], int delPlc, char del[], int rowN, int colN){
  for(int i = 1; i < argc; i++){
    char *strtolptr = "";
    if(i != delPlc && !strcmp(argv[i], "irow") && rowN == (strtol(argv[i+1], &strtolptr, 10))){
      for(int j = 0; j <= colN; j++)
        printf("%c", del[0]);
      printf("\n");
    }
  }
}

int drows(int argc, char *argv[], int delPlc, int rowN){
  if(rowN < 0)
    rowN = -rowN;
  for(int i = 1; i < argc; i++){
    if(i != delPlc){
      if(!strcmp(argv[i], "drow") || !strcmp(argv[i], "drows")){
        char *strtolptr = "";
        int arg1 = strtol(argv[i+1], &strtolptr, 10);
        if(strtolptr[0])
          return 2;
        if(!strcmp(argv[i], "drow") && rowN == arg1)
          return 1;
        if(!strcmp(argv[i], "drows")){
          char *strtolptr2 = "";
          int arg2 = strtol(argv[i+2], &strtolptr2, 10);
          if(strtolptr2[0])
            return 2;
          if(rowN >= arg1 && rowN <= arg2)
            return 1;
        }
      }
    }
  }
  return 0;
}

bool dcols(int argc, char *argv[], int colN, int delPlc){
  char *strtolptr;
  for(int i = 0; i < argc; i++){
    if(i != delPlc){
      if(!strcmp(argv[i], "dcol") && colN == strtol(argv[i+1], &strtolptr, 10))
          return strtol(argv[i+1], &strtolptr, 10);
      else if(!strcmp(argv[i], "dcols"))
        if(colN >= strtol(argv[i+1], &strtolptr, 10) && colN <= strtol(argv[i+2], &strtolptr, 10))
          return true;
    }
  }
  return false;
}

void arow(int argc, char *argv[], int delPlc, char del[], int colN){
  for(int i = 1; i < argc; i++){
    if(i != delPlc && !strcmp(argv[i], "arow")){
      for(int j = 0; j <= colN; j++)
        printf("%c", del[0]);
      printf("\n");
    }
  }
}

bool getSlAndPcC(int argc, char *argv[], char *pcC, char pcCArr[][mxCL], 
    char pcCAs[mxCAsN][mxSL], int pcCAN[], size_t pcCANCt, 
    char slC[mxCL], char slCArr[][mxCL], char slCAs[][mxSL], int slCN){
  for(int i = 1; i < argc; i++){
    for(size_t j = 0; j < pcCANCt; j++){
      if(!strcmp(argv[i], pcCArr[j])){
        strcpy(pcC, argv[i]);
        for(int k = 0; k < pcCAN[j]; k++)
          if((i+pcCAN[j]) <= argc)
            strcpy(pcCAs[k], argv[i+k+1]); //i+k is the place of the command name. i+k+1 is the first command argument
        if(slCN == 1){
          if(!strcmp(argv[i-3], slCArr[0]) || //argv[i-3] is supposed to be a name of a slection command
              !strcmp(argv[i-3], slCArr[1]) || 
              !strcmp(argv[i-3], slCArr[2])){  //.. so we are making sure it really is.
            strcpy(slC, argv[i-3]); //and then writing the typed command into slC variable
            strcpy(slCAs[0], argv[i-2]); //and finally, writing arguments of the command to slCAs variable
            strcpy(slCAs[1], argv[i-1]);
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

int pcAndSlCFn(int argc, char *argv[], int delPlc, int *edCN, int *pcCN, 
    char pcC[mxCL], char pcCAs[mxCAsN][mxSL], char slC[mxCL], char slCAs[3][mxSL]){
  char pcCArr[][mxCL] = {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move", 
      "csum", "cavg", "cmin", "cmax", "ccount", "cseq", "rseq", "rsum", "ravg", "rmin", "rmax", "rcount"};
  int  pcCAN[] = {2, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3, 3};
  char slCArr[][mxCL] =  {"rows", "beginswith", "contains"};
  int  slCN = 0;
  size_t pcCANCt = sizeof(pcCAN) / sizeof(int);
  for(int i = 1; i < argc; i++){
    if(i == (delPlc - 1))
      i+=2; //if the program approaches -d, it skips it along with the next arg (delimiters themslf)
    int edCAN[] = {1, 0, 1, 2, 1, 0, 1, 2};
    int slCAN[] = {2, 2, 2};
    char edCArr[][mxCL] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"};
    if(!isC(argc, argv, delPlc, i, edCAN, edCArr, edCN)
      || !isC(argc, argv, delPlc, i, pcCAN, pcCArr, pcCN)
      || !isC(argc, argv, delPlc, i, slCAN, slCArr, &slCN))
      return 2;
  }
  if(*pcCN == 1)
    if(!getSlAndPcC(argc, argv, pcC, pcCArr, pcCAs, pcCAN, pcCANCt, slC, slCArr, slCAs, slCN))
      return 5;
  if(*pcCN > 1 || slCN > 1){
    fprintf(stderr, "ONLY ONE PROCESSING/SELECTION COMMAND IS ACCEPTABLE (OR NONE)! THE PROGRAM WILL NOW EXIT.\n");
    return 3;
  }
  if((*pcCN || slCN) && *edCN){
    fprintf(stderr, "YOU CANNOT EDIT AND PROCESS THE TABLE IN ONE EXECUTION! THE PROGRAM WILL NOW EXIT.\n");
    return 4; //In case of there being a table pcessing command and also a table eding command 
  }
  return 1;
}


int main(int argc, char *argv[]){
  if(argc > mxC){
    fprintf(stderr, "TOO MANY ARGUMENTS! THE PROGRAM WILL NOW EXIT.\n");
    return 1;
  }
  if(!checkArgs(argc, argv))
    return 1;
  int  edCN = 0;
  int  pcCN = 0;
  char pcC[mxCL]; //for the name of a pcessing command (longest one is toupper, which has 7 chars)
  char pcCAs[mxCAsN][mxSL];
  char slC[mxCL]; //for the name of a slection command (longest one is beginswith, which has 10 chars)
  char slCAs[3][mxSL]; //there are two arguments for each slection command
  char del[2];
  int delCt = 0;
  int delPlc = getDelCt(argc, argv, &delCt); //Get info about where the deliter chars are in argv
  char delArr[delCt + 1];
  getDel(&delCt, argv, delArr, delPlc);
  del[0] = delArr[0];
  if(1){
    int tempRet = pcAndSlCFn(argc, argv, delPlc, &edCN, &pcCN, pcC, pcCAs, slC, slCAs);
    if(tempRet != 1)
      return tempRet;
  }
  char prevBuffer[mxRwL]; //necessary because I don't know that a line is the last one, until I load it into a buffer
  char buffer[mxRwL]; //lines will be temporarily saved here (one at a time) 
  char token[mxSL]; //generally, cells will be temporarily saved here (one at a time)
  int rowN = 0; //keeping track of which line is the program actually pcessing (negative number for the last row)
  int finish = 0;
  int colN = 0; //keeping track of which column is the program actually pcessing
  int prevColN = 1; //keeps track of total columns of the previous line, which is used to check, if all the table lines have the same amount of columns
  fgets(prevBuffer, mxRwL, stdin);//load a line into the buffer. If the line is empty, it means the previous row was the last one
  double rSAMMCVal = 0;
  int rSAMMCAvgCt = 0;
  while(1){
    strcpy(buffer, prevBuffer);
    if(!strchr(buffer, '\n')){
      fprintf(stderr, "ONE OR MORE LINES OF YOUR TABLE ARE TOO LONG! THE PROGRAM WILL NOW EXIT.\n");
      return 10;
    }
    if(!fgets(prevBuffer, mxRwL, stdin) && !finish){ //load a line into the buffer. If the line is empty, it means the previous row was the last one
      rowN = -rowN; //to indicate that this is the last row, we set rowN to negative (last run of the while cycle)
      finish++;
      continue;
    }
    else if(finish == 2)
      break;
    else if(finish)
      finish++;

    char tempToken[mxSL];
    int l = 0;
    if(rowN >= 0)
      rowN++;
    else
      rowN--;
    for(int i = 0; buffer[i]; i++)
      for(int j = 0; j < delCt; j++)
        if(buffer[i] == delArr[j]){
          buffer[i] = del[0];
          if(colN == 0)
            prevColN++;
        }
    irow(argc, argv, delPlc, del, rowN, prevColN);
    int drowsRet = drows(argc, argv, delPlc, rowN);
      if(drowsRet == 2)
        return 0;
      else if(drowsRet == 1){
        colN = prevColN;
        continue;
      }
    colN = 0;
    bool dontPtNextDel = false;
    double cSAMMCVal = 0; //column SumAvgMinMaxCount
    for(size_t k = 0; k < strlen(buffer); k++){
      //TODO vsetko vo forku do funkcie
      bool dontPtNextChar = false;
      if(buffer[k] == del[0])
        if(k == 0 || buffer[k-1] == del[0])
          token[l++] = 0;
      if(buffer[k] != del[0] && buffer[k] != '\n'){
        token[l] = buffer[k];
        token[++l] = 0;
      }else{
        colN++;
        if(pcCN == 1){
          int pcTheTabRet = pcTheTab(buffer, del, token, tempToken, rowN, colN, pcC, pcCAs, slC, slCAs, &cSAMMCVal, &rSAMMCVal, &rSAMMCAvgCt);
          if(!pcTheTabRet){
            fprintf(stderr, "THE ARGUMENTS YOU PROVIDED CANNOT BE WORKED WITH! THE PROGRAM WILL NOW EXIT.\n");
            return 6; //In case of invalid arguments. e.g if an arg is supposed to be a number but is a character
          }
          if(pcTheTabRet == 2)
            dontPtNextChar = true;
          if(pcTheTabRet == 2 || pcTheTabRet == 3)
            dontPtNextDel = true;
          if(pcTheTabRet == 4 || pcTheTabRet == 6){
            double outVar = cSAMMCVal;
            double tempVar = cSAMMCVal;
            if(pcTheTabRet == 6)
              tempVar = outVar = rSAMMCVal;
            while(tempVar >= 1)
              tempVar--;
            if(tempVar > 0 && tempVar < 1)
              sprintf(token, "%g", outVar); 
            else
              sprintf(token, "%d", (int)outVar);
          }
          if(pcTheTabRet == 5)
            strcpy(token, "NaN");
        }else if(edCN)
          icolAcol(token, colN, prevColN, argc, argv, delPlc, del);
        if(dcols(argc, argv, colN, delPlc)){
          dontPtNextChar = true;
          if(colN == 1)
            dontPtNextDel = true;
        }
        if(colN && !strchr(token, '\n') && !dontPtNextChar){
          if(colN > 1 && !dontPtNextDel)
            printf("%c", del[0]);
          printf("%s", token);
          token[0] = 0;
          dontPtNextDel = false;
        }
        l = 0;
      }
    }
    printf("\n");
    if(prevColN != colN && rowN >= 0){
      fprintf(stderr, "NOT ALL ROWS HAVE THE SAME NUMBER OF COLUMNS! THE PROGRAM WILL NOW EXIT.\n");
      return 7; //if there are two or more rows with different numbers of columns
    }
  }
  arow(argc, argv, delPlc, del, colN);
  return 0;
}

