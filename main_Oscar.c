/*
** The table eding commands put in by a user do not run in the order that the user typed them.
**
** Variables:
** del is a delimiter
** edC is an eding command
** pcC is a pcessing command
** slC is a slection command
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
** ct = count
**
** Return values from main:
** 0 everything went well
** 1 one of the arguments is too long
** 2 there are too many arguments entered
** 3 there are not enough arguments for a command or an argument is supposed to be a number but is not
** 4 an argument is an unacceptable number
** 5 problem with a selection command 
** 6 more than one table processing command or selection command was entered
** 7 one or more table processing or selection commands and one or more table 
**   editing commands were entered simultaneously
** 8 one or more rows of the table exceed maximum row size (10KiB)
** 9 not all rows of the table have the same amount of columns
** 10 using a row selection command with rseq, rsum, ravg, rmin, rmax or rcount
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define mxRwL 10242 //mx length of a row is 10KiB
#define mxSL 101   //mx cell (string) length is 100 characters
#define mxC 101     //max number of arguments in one run of the program
#define mxAL 101  //max argument length
#define mxCL 11    //length of the longest command
#define mxCAsN 6 //no command is supposed to have 5 or more arguments, but the cmd name will be stored with them as well

void swapArgs(int *arg1, int *arg2){ //function to swap two arguments if one is bigger than the other
  if(*arg1 > *arg2){
    int tempVal = *arg1;
    *arg1 = *arg2;
    *arg2 = tempVal;
  }
}

int getDelPlace(int argc, char *argv[]){
  if(argc > mxC){
    fprintf(stderr, "YOU ENTERED TOO MANY ARGUMENTS! THE MAXIMUM AMOUNT IS 100! THE PROGRAM WILL NOW EXIT.\n");
    return -2;
  }
  for(int i = 1; i < (argc - 1); i++){
    for(int j = 0; argv[i][j]; j++)
      if(j >= mxAL){
        fprintf(stderr, "ONE OF YOUR ARGUMENTS IS TOO LONG! THE MAXIMUM LENGTH IS 100! THE PROGRAM WILL NOW EXIT.\n"); 
        return -1;
      }
    if(strstr(argv[i], "-d"))
      return (i + 1);
  }
  return 0;
}

void replaceDelims(char buffer[mxRwL], char *del, int colN, int *prevColN){
  for(int i = 0; buffer[i]; i++)
    for(int j = 0; del[j]; j++)
      if(buffer[i] == del[j]){
        buffer[i] = del[0];
        if(colN == 0)
          (*prevColN)++;
      }
}

bool isC(int argc, char *argv[], int delPlc, int j, int cmdAsN[], char cmdArr[][mxCL], int *cmdN){
  char exceptions[][mxCL] = {"cset", "beginswith", "contains"};//the only ones that can have a string as an argument
  int numOfExceptions = 3;
  if(argv[j]){
    for(int i = 0; cmdArr[i][0]; i++){
      if(!strcmp(argv[j], cmdArr[i])){
        if((j + cmdAsN[i]) >= argc || (delPlc > j && delPlc <= (j + cmdAsN[i] + 1))){
          fprintf(stderr, "THERE ARE NOT ENOUGH ARGUMENTS FOR YOUR COMMAND '%s'! THE PROGRAM WILL NOW EXIT.\n", cmdArr[i]);
          return false;
        }
        for(int l = 0; l < cmdAsN[i]; l++){
          char *strtolptr;
          strtol(argv[j + l + 1], &strtolptr, 10);
          if((strtolptr[0] && strtolptr[0] != '-') 
              || (strtolptr[0] == '-' && strcmp(argv[j], "rows") && !((!strcmp(argv[j], "rseq") && l == 2)))){  
                  //- is allowed when using rows (on both positions) or rseq (at the third place)
            fprintf(stderr, "ARGUMENT/ARGUMENTS FOLLOWING YOUR COMMAND '%s' IS/ARE SUPPOSED TO BE A NUMBER! THE PROGRAM WILL NOW EXIT.\n", cmdArr[i]);
            return false;
          }
          for(int m = 0; m < numOfExceptions; m++)
            if(!strcmp(cmdArr[i], exceptions[m]) && l == 0) //all exceptions can only have string as the second arg
              l = cmdAsN[i];
        }
        (*cmdN)++;
        return 1;
      }
    }
  }
  return 1;
}

bool cpSpMvFn(char buffer[mxRwL], char token[mxSL], char *del, int pcCA2, bool move, bool reverseAs){
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
        strncat(theOtherToken, del, 1);
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
int cpSpMv(char pcC[mxCAsN][mxSL], int pcCA1, int pcCA2, char *token, char *tempToken, int colN, char buffer[mxRwL], char *del){ 
  int maxC = 1;
  for(int i = 0; buffer[i]; i++)
    if(buffer[i] == del[0])
      maxC++;
  bool copy = !strcmp(pcC[0], "copy");
  bool move = !strcmp(pcC[0], "move");
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
    if((copy && !reverseAs) || cpSpMvFn(buffer, token, del, pcCA2, move, reverseAs))
      return 1;
    if(move && !reverseAs)
      return 2;
  }else{
    if(move && reverseAs){
      token[0] = '\0';
      return 3;
    }else if(move && !reverseAs){
      strncat(tempToken, del, 1);
      strcat(tempToken, token);
      strcpy(token, tempToken);
    }else if(copy && reverseAs)
      return 1;
    strcpy(token, tempToken);
  }
  return 1;
}

int doIEdFn(char slC[mxCAsN][mxSL], char token[mxSL], int rowN, int colN){
  bool doIEd = false;
  char *strtolptr;
  char *strtolptr2;
  int arg1 = strtol(slC[1], &strtolptr, 10);
  int arg2 = strtol(slC[2], &strtolptr2, 10);
  if(!slC[0][0])
    doIEd = true;
  else if(!strcmp(slC[0], "rows")){
    if(slC[2][0] == '-'){
      if(slC[1][0] == '-'){ 
        if(rowN < 0)
          doIEd = true;
      }else if(rowN >= arg1 || (rowN < 0 && -rowN >= arg1))
        doIEd = true;
    }else if(arg1 > arg2){
      swapArgs(&arg1, &arg2);
    }else{
      if(rowN < 0)
        rowN = -rowN;      
      if(rowN >= arg1 && rowN <= arg2)
        doIEd = true;
    }
  }else if(!strcmp(slC[0], "beginswith") && colN == arg1 && token == strstr(token, slC[2]))
    doIEd = true;     
  else if(!strcmp(slC[0], "contains") && colN == arg1 && strstr(token, slC[2]))
    doIEd = true;     
  /*if(slC[0] && strtolptr[0] && strtolptr[0] != '-')
    return 2;*/
  if(doIEd == true)
    return 1;
  return 0;
}

int cSAMMCFn(double *cSAMMCVal, char buffer[mxRwL], char *del, char pcC[mxCAsN][mxSL], int colN){
  char cs[][mxCL] = {"csum", "cavg", "cmin", "cmax", "ccount"}; //commands
  int csNum = 5;
  double outVal = 0;
  char *strtolptr/* = ""*/;
  for(int i = 0; i < csNum; i++)
    if(!strcmp(pcC[0], cs[i]) && strtol(pcC[1], &strtolptr, 10)){
      /*if(strtolptr[0]){
        fprintf(stderr, "THE ARGUMENT %s YOU ENTERED WITH YOUR COMMAND %s MUST BE A NUMBER! THE PROGRAM WILL NOW EXIT.\n", pcC[1], pcC[0]);
        return ;
      }*/
      bool retNULL = true; //If value of this bool isnt changed during this cycle and retNULLHelper will be...
      bool retNULLHelper = false; //...set to true as well, "NaN" should be written to the cell instead of a number
      int arg1 = strtol(pcC[1], &strtolptr, 10);
      if(arg1 == colN){
        char *strtolptr2/* = ""*/;
        char *strtolptr3/* = ""*/;
        int arg2 = strtol(pcC[2], &strtolptr2, 10);
        int arg3 = strtol(pcC[3], &strtolptr3, 10);
        swapArgs(&arg2, &arg3);//if arg2 > arg3, swap them
        if((arg1 >= arg2 && arg1 <= arg3) || strtolptr2[0] || strtolptr3[0]){
          fprintf(stderr, "THE DESTINATION COLUMN FOR YOUR COMMAND '%s' CANNOT BE WITHIN THE RANGE OF THE OPERATION! THE PROGRAM WILL NOW EXIT.\n", pcC[0]);
          return -4;
        }
        bool init = false;
        int actCol = 1;
        int avgCt = 0;
        int k = 0;
        char tempTok[mxSL]/* = ""*/;
        for(int j = 0; j < (int)strlen(buffer); j++){
          if(buffer[j] == del[0] || buffer[j] == '\n'){ //if we approached a delimiter or a line break, it means an end of a cell
            if(actCol >= arg2 && actCol <= arg3){
              tempTok[k] = '\0';
              char *strtodptr/* = ""*/;
              double tokVal = strtod(tempTok, &strtodptr);
              if(!strcmp(pcC[0], "csum"))
                outVal = outVal + tokVal; 
              else if(!strcmp(pcC[0], "cavg")){
                outVal += tokVal;
                avgCt++;
                retNULLHelper = true;
                if(!strtodptr[0])
                  retNULL = false; //if at least one of processed cells is a number, there will be a number written to the table
              }else if(!strcmp(pcC[0], "cmin") || !strcmp(pcC[0], "cmax")){
                retNULLHelper = true;
                if(!strtodptr[0]) //if at least one of processed cells is a number, there will be a number written to the table
                  retNULL = false;
                if(!init && !strtodptr[0]){ //if this is the first cycle (!init) and there is a number in the column, copy it.
                  outVal = tokVal;
                  init = true;
                }else if(!strcmp(pcC[0], "cmin") && outVal > tokVal)
                  outVal = tokVal;
                else if(!strcmp(pcC[0], "cmax") && outVal < tokVal)
                  outVal = tokVal;
              }else if(!strcmp(pcC[0], "ccount"))
                if(!strtodptr[0])
                  outVal++;
            }
            actCol++;
            k = 0;
          }
          else if(actCol >= arg2 && actCol <= arg3) //if the actual column should be processed...
            tempTok[k++] = buffer[j]; //...keep writing characters from buffer (mind the k++)
        }
        if(!strcmp(pcC[0], "cavg"))
          outVal /= avgCt; //divide the sum by how many numbers were summed so we get an average
        *cSAMMCVal = outVal; //write the output value to the pointer so it can be printed outside this function.
        if(retNULL && retNULLHelper)
          return 6; //no number should be written into the table. Write "NaN" instead (will be specified later).
        return 4; //write the value to the table
      }
    }
  return 1; //if no changes have been made
}

int cseq(int colN, char pcC[mxCAsN][mxSL], double *cSAMMCVal){
  int arg1 = atoi(pcC[1]);
  int arg2 = atoi(pcC[2]);
  double arg3 = atol(pcC[3]);
  swapArgs(&arg1, &arg2); //if arg1 > arg2, swap them
  if(colN == arg1)
    *cSAMMCVal = arg3;
  else if(colN > arg1 && colN <= arg2)
    (*cSAMMCVal)++;
  else
    return 1; //no changes
  return 4; //write the value into the table
}

int rseq(char pcC[mxCAsN][mxSL], int rowN, int colN, double *rSAMMCVal){
  char *strtolptr[4];
  int arg[4];
  for(int i = 0; i < 4; i++){
    arg[i] = strtol(pcC[i+1], &strtolptr[i], 10);
    /*if((strtolptr[i][0] && i != 2) || (i == 2 && strtolptr[i][0] && strtolptr[i][0] != '-')){ //arg can be a - if it is the third arg
      fprintf(stderr, "ALL ARGUMENTS BELONGING TO THE COMMAND %s HAVE TO BE NUMBERS (OR -, WHICH CAN BE EXPECTED AS THE THIRD ARGUMENT)! THE PROGRAM WILL NOW EXIT. \n", pcC[0]);
      return ;
    }*/
  }
  if(rowN < 0)
    rowN = 0 - rowN;
  if(strtolptr[2][0] == '-')
    arg[2] = rowN + 1; //if there is a - as the third arg, rseq will work for every line from arg2
  swapArgs(&arg[1], &arg[2]); //if arg1 > arg2, swap them
  if(colN == arg[0] && rowN >= arg[1] && rowN <= arg[2]){
    if(rowN == arg[1])
      *rSAMMCVal = arg[3]; 
    else if(rowN > arg[1] && rowN <= arg[2])
      (*rSAMMCVal)++;
    return 5; //write the value
  }
  return 1; //no changes
}

int rSAMMCFn(char token[mxSL], char pcC[mxCAsN][mxSL], int colN, int rowN, double *rSAMMCVal, int *rSAMMCAvgCt){
  char cs[][mxCL] = {"rsum", "ravg", "rmin", "rmax", "rcount"}; //commands
  int csNum = 5;
  bool edit;
  for(int i = 0; i < csNum; i++)
    if(!strcmp(pcC[0], cs[i]))
      edit = true;
  if(!edit)
    return 1;
  if(rowN < 0)
    rowN = 0 - rowN;
  int arg1 = atoi(pcC[1]);
  if(colN == arg1){
    char *strtodptr/* = ""*/;
    double tokVal = strtod(token, &strtodptr);
    int arg2 = atoi(pcC[2]);
    int arg3 = atoi(pcC[3]);
    if(rowN >= arg2 && rowN <= arg3){
      if(strtodptr[0])
        return 1;
      if(!strcmp(pcC[0], "rsum")){
        *rSAMMCVal += tokVal;
      }else if(!strcmp(pcC[0], "ravg")){
        *rSAMMCVal += tokVal;
        (*rSAMMCAvgCt)++;
      }else if(!strcmp(pcC[0], "rmin") || !strcmp(pcC[0], "rmax")){
        if(*rSAMMCAvgCt == 0){
          *rSAMMCVal = tokVal;
          (*rSAMMCAvgCt)++;
        }else if(!strcmp(pcC[0], "rmin") && tokVal < *rSAMMCVal)
          *rSAMMCVal = tokVal;
        else if(!strcmp(pcC[0], "rmax") && tokVal > *rSAMMCVal)
          *rSAMMCVal = tokVal;
      }else if(!strcmp(pcC[0], "rcount"))
        (*rSAMMCVal)++;
    }
    if(rowN == arg3 + 1){ //if the actual row is the one to write the output to
      if(!strcmp(pcC[0], "ravg"))
        *rSAMMCVal /= (*rSAMMCAvgCt);
      //for commands rmin, rmax and ravg - if none of the processed cells was a cell
      if(*rSAMMCAvgCt == 0 && (!strcmp(pcC[0], "rmin") || !strcmp(pcC[0], "rmax") || !strcmp(pcC[0], "ravg")))
        return 6;
      return 5;
    }
  }
  return 1;
}

bool pcNumsAndChars(char pcC[mxCAsN][mxSL], char token[mxSL]){
  if(!strcmp(pcC[0], "tolower")){
    for(size_t i = 0; i < strlen(token); i++)
      if(token[i] >= 'A' && token[i] <= 'Z')
        token[i] += 'a' - 'A';
  }else if(!strcmp(pcC[0], "toupper")){
    for(size_t i = 0; i < strlen(token); i++)
      if(token[i] >= 'a' && token[i] <= 'z')
        token[i] -= 'a' - 'A';
  }else if(!strcmp(pcC[0], "round")){
    char *strtodptr;
    double num = 0.5 + strtod(token, &strtodptr);
    int intN = num;
    if(!strtodptr[0])
      sprintf(token, "%d", intN);
  }else if(!strcmp(pcC[0], "int")){
    bool isNber = true;
    for(size_t i = 0; i < strlen(token); i++)
      if((token[i] < '0' || token[i] > '9') && token[i] != '.')
        isNber = false;
    if(isNber)
      sprintf(token, "%d", (int)atol(token));
  }else
    return false;
  return true;
}

int pcTheTab(char buffer[], char *del, char token[mxSL], char tempToken[mxSL], int rowN, int colN, 
    char pcC[mxCAsN][mxSL], char slC[mxCAsN][mxSL], double *cSAMMCVal, double *rSAMMCVal, int *rSAMMCAvgCt){
  int doIEdFnRet = doIEdFn(slC, token, rowN, colN);
  //if(doIEdFnRet == 2)
    //return 0;
  if(doIEdFnRet == 1){
    int cSAMMCFnRet = cSAMMCFn(cSAMMCVal, buffer, del, pcC, colN);
    int rSAMMCFnRet = rSAMMCFn(token, pcC, colN, rowN, rSAMMCVal, rSAMMCAvgCt);
    if(cSAMMCFnRet < 0)
      return cSAMMCFnRet; //err
    if(rSAMMCFnRet < 0)
      return rSAMMCFnRet; //err
    if(cSAMMCFnRet > 1)
      return cSAMMCFnRet;
    if(rSAMMCFnRet > 1)
      return rSAMMCFnRet;
    /*if(cSAMMCFnRet == 6 || rSAMMCFnRet == 6) //write NaN to the table instead of a number
      return 6;
    if(cSAMMCFnRet == 4) //write the output to the table
      return 4;
    if(rSAMMCFnRet == 5) //write the output to the table
      return 5;*/
    if(!strcmp(pcC[0], "cseq"))
      return cseq(colN, pcC, cSAMMCVal);
    if(!strcmp(pcC[0], "rseq"))
      return rseq(pcC, rowN, colN, rSAMMCVal);
    char *strtolptr;
    int pcCA1 = strtol(pcC[1], &strtolptr, 10);
    if(strtolptr[0])
      return 0;
    if(colN == pcCA1){
      if(!strcmp(pcC[0], "cset"))
        strcpy(token, pcC[2]);
      else if(pcNumsAndChars(pcC, token))
        return 1;
    }
    if(!strcmp(pcC[0], "copy") || !strcmp(pcC[0], "swap") || !strcmp(pcC[0], "move")){
      char *strtolptr2;
      int pcCA2 = strtol(pcC[2], &strtolptr2, 10);
      if(strtolptr2[0])
        return 0;
      if(pcCA1 != pcCA2)
        if(colN == pcCA1 || colN == pcCA2)
          return cpSpMv(pcC, pcCA1, pcCA2, token, tempToken, colN, buffer, del); 
    }
  }
  return 1;
}

void icolAcol(char *token, long int colN, int prevColN, int argc, char *argv[], int delPlc){
  char *strtolptr;
  for(int i = 1; i < argc; i++)
    if(i != delPlc){
      if(!strcmp(argv[i], "icol") && colN == strtol(argv[i+1], &strtolptr, 10)){
        char tempDel[mxSL+1];
        tempDel[0] = argv[delPlc][0];
        tempDel[1] = '\0';
        strncat(tempDel, token, mxSL);
        strcpy(token, tempDel);
      }else if(!strcmp(argv[i], "acol"))
        if(colN == prevColN)
          strncat(token, argv[delPlc], 1);
    }
}

void irow(int argc, char *argv[], int delPlc, int rowN, int colN){
  for(int i = 1; i < argc; i++)
    if(i != delPlc && !strcmp(argv[i], "irow") && rowN == atoi(argv[i+1])){
      for(int j = 0; j <= colN; j++)
        printf("%c", argv[delPlc][0]);
      printf("\n");
    }
}

bool drows(int argc, char *argv[], int delPlc, int rowN){
  if(rowN < 0)
    rowN = -rowN;
  for(int i = 1; i < argc; i++){
    if(i == delPlc)
      continue;
    if((!strcmp(argv[i], "drow") && rowN == atoi(argv[i+1]))
      || (!strcmp(argv[i], "drows") && rowN >= atoi(argv[i+1]) && rowN <= atoi(argv[i+2])))
      return 1;
  }
  return 0;
}

bool dcols(int argc, char *argv[], int colN, int delPlc){
  for(int i = 0; i < argc; i++)
    if(i != delPlc){
      if(!strcmp(argv[i], "dcol") && colN == atoi(argv[i+1]))
          return atoi(argv[i+1]);
      if(!strcmp(argv[i], "dcols") && colN >= atoi(argv[i+1]) && colN <= atoi(argv[i+2]))
          return true;
    }
  return false;
}

void arow(int argc, char *argv[], int delPlc, int colN){
  for(int i = 1; i < argc; i++)
    if(i != delPlc && !strcmp(argv[i], "arow")){
      for(int j = 0; j <= colN; j++)
        printf("%c", argv[delPlc][0]);
      printf("\n");
    }
}

//this function searches for processing and selection commands. If found, writes them into pcC and slC
bool getSlAndPcC(int argc, char *argv[], char pcC[mxCAsN][mxSL], char pcCArr[][mxCL],
    int pcCAN[], char slC[mxCAsN][mxSL], char slCArr[][mxCL], int slCN){
  for(int i = 1; i < argc; i++)
    for(int j = 0; pcCAN[j]; j++) //pcCANCt is array with a number of args for each cmd. Iterations = it's length
      if(!strcmp(argv[i], pcCArr[j])){
        strcpy(pcC[0], argv[i]);
        for(int k = 0; k < (pcCAN[j] + 1); k++) //pcCAN[j] is a number saying how many arguments should there be for the cmd
          /*if((i+pcCAN[j]) <= argc)*/
            strcpy(pcC[k], argv[i+k]); //i+k is the place of the command name. i+k+1 is the first command argument
        if(slCN == 1){ 
          if(!strcmp(argv[i-3], slCArr[0]) || !strcmp(argv[i-3], slCArr[1]) || !strcmp(argv[i-3], slCArr[2])){ 
            //argv[i-3] is supposed to be a name of a slection command, so we are making sure it really is.
            strcpy(slC[0], argv[i-3]); //and then writing the typed command into slC variable
            strcpy(slC[1], argv[i-2]); //and finally, writing arguments of the command to slCAs variable
            strcpy(slC[2], argv[i-1]);
          }else{
            fprintf(stderr, "THERE IS A PROBLEM WITH YOUR SELECTION COMMAND! CHECK THE ARGUMENTS AND THE PLACE OF THE COMMAND! THE PROGRAM WILL NOW EXIT.\n");
            return false;
          }
        }
      }
  return true;
}

int pcAndSlCFn(int argc, char *argv[], int delPlc, int *edCN, int *pcCN, 
    char pcC[mxCAsN][mxSL], char slC[3+1][mxSL]){
  char pcCArr[][mxCL] = {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move", 
      "csum", "cavg", "cmin", "cmax", "ccount", "cseq", "rseq", "rsum", "ravg", "rmin", "rmax", "rcount"};
  int pcCAN[] = {2, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 0}; //0 is there just to know the end
  char slCArr[][mxCL] =  {"rows", "beginswith", "contains"};
  int slCN = 0;
  for(int i = 1; i < argc; i++){
    if(i == (delPlc - 1))
      i+=2; //if the program approaches -d, it skips it along with the next arg (delimiters themslf)
    int edCAN[] = {1, 0, 1, 2, 1, 0, 1, 2};
    int slCAN[] = {2, 2, 2};
    char edCArr[][mxCL] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"};
    if(!isC(argc, argv, delPlc, i, edCAN, edCArr, edCN)
      || !isC(argc, argv, delPlc, i, pcCAN, pcCArr, pcCN)
      || !isC(argc, argv, delPlc, i, slCAN, slCArr, &slCN))
      return -3;
  }
  if(*pcCN == 1)
    if(!getSlAndPcC(argc, argv, pcC, pcCArr, pcCAN, slC, slCArr, slCN))
      return -5;
  if(*pcCN > 1 || slCN > 1){
    fprintf(stderr, "ONLY ONE PROCESSING/SELECTION COMMAND IS ACCEPTABLE (OR NONE)! THE PROGRAM WILL NOW EXIT.\n");
    return -6;
  }
  if((*pcCN || slCN) && *edCN){
    fprintf(stderr, "YOU CANNOT EDIT AND PROCESS THE TABLE IN ONE EXECUTION! THE PROGRAM WILL NOW EXIT.\n");
    return -7; //In case of there being a table pcessing command and also a table eding command 
  }
  if(*pcCN && slCN && pcC[0][0] == 'r'){
    fprintf(stderr, "IT IS FORBIDDEN TO USE A ROW SELECTION COMMAND ALONG WITH YOUR '%s' COMMAND! THE PROGRAM WILL NOW EXIT.\n", pcC[0]);
    return -10;
  }
  return 1;
}


int main(int argc, char *argv[]){
  int delPlc = getDelPlace(argc, argv); //Get info about where the deliter chars are in argv
  char *del = " ";
  if(delPlc < 0) //if -1 is returned, one of the arguments is too long. For -2 there are too many arguments
    return -getDelPlace(argc, argv); //getDelPlace will return -1 or -2, so from main, we return 1 or 2
  if(delPlc > 0)
    del = argv[delPlc]; //if there are arguments specified, del will point to delimiters in argv
  int  edCN = 0; //amount of editing commands
  int  pcCN = 0; //amount of processing commands
  char pcC[mxCAsN][mxSL]; //storing the name and arguments of a processing command
  char slC[mxCAsN][mxSL];
  int tempRet = pcAndSlCFn(argc, argv, delPlc, &edCN, &pcCN, pcC, slC);
  if(tempRet != 1)
    return -tempRet;
  char prevBuffer[mxRwL]; //necessary because I don't know that a line is the last one, until I load it into a buffer
  char buffer[mxRwL]; //lines will be temporarily saved here (one at a time) 
  char token[mxSL]; //generally, cells will be temporarily saved here (one at a time)
  int rowN = 0; //keeping track of which line is the program actually pcessing (negative number for the last row)
  int prevColN = 1; //keeping track of total columns of the previous line
  int colN = 0; //keeping track of which column is the program actually pcessing
  int finish = 0; //gets used when fgets returns EOF and we need to process the last line
  double rSAMMCVal = 0;
  int rSAMMCAvgCt = 0;
  fgets(prevBuffer, mxRwL, stdin);
  while(1){
    strcpy(buffer, prevBuffer); //load the line into the buffer
    if(!strchr(buffer, '\n')){
      fprintf(stderr, "THE ROW NUMBER '%d' OF YOUR TABLE IS TOO LONG! THE PROGRAM WILL NOW EXIT.\n", rowN);
      return 8;
    }
    if(!fgets(prevBuffer, mxRwL, stdin) && !finish){ //load a line into the buffer. If the line is empty, it means the previous row was the last one
      rowN = -rowN; //to indicate that this is the last row, we set rowN to negative (last run of the while cycle)
      finish++;
      continue;
    }else if(finish == 2)
      break;
    else if(finish)
      finish++;
    if(rowN >= 0)
      rowN++;
    else
      rowN--;
    replaceDelims(buffer, del, colN, &prevColN);
    irow(argc, argv, delPlc, rowN, prevColN);
    if(drows(argc, argv, delPlc, rowN)){
      colN = prevColN;
      continue;
    }
    colN = 0;
    int l = 0;
    bool dontPtNextDel = false;
    double cSAMMCVal = 0; //column SumAvgMinMaxCount
    char tempToken[mxSL];
    for(size_t k = 0; k < strlen(buffer); k++){
      bool dontPtNextChar = false;
      if(buffer[k] == argv[delPlc][0] && (k == 0 || buffer[k-1] == argv[delPlc][0])) //if the first char of a line is a delim or previous char was a delim 
        token[l++] = 0; //write terminal null as the last tokens char and increase l by one
      if(buffer[k] != argv[delPlc][0] && buffer[k] != '\n'){ //if a char isn't a delim and neither a line break
        token[l] = buffer[k]; //copy a char from the buffer
        token[++l] = 0; //and also increase the l by one, and finally, put terminal null after the character
      }else{
        colN++;
        if(pcCN == 1){ //if a table processing command was inputed by the user
          int pcTheTabRet = pcTheTab(buffer, del, token, tempToken, rowN, colN, pcC, slC, &cSAMMCVal, &rSAMMCVal, &rSAMMCAvgCt);
          if(pcTheTabRet < 0){
            //fprintf(stderr, "THE ARGUMENTS YOU PROVIDED CANNOT BE WORKED WITH! THE PROGRAM WILL NOW EXIT.\n"); //In case of invalid arguments. e.g if an arg is supposed to be a number but is a character
            return -pcTheTabRet; 
          }
          if(pcTheTabRet == 2)
            dontPtNextChar = true;
          if(pcTheTabRet == 2 || pcTheTabRet == 3)
            dontPtNextDel = true;
          if(pcTheTabRet == 4 || pcTheTabRet == 5){
            double outVar = cSAMMCVal;
            double tempVar = cSAMMCVal;
            if(pcTheTabRet == 5)
              tempVar = outVar = rSAMMCVal;
            while(tempVar >= 1)
              tempVar--;
            if(tempVar > 0 && tempVar < 1)
              sprintf(token, "%g", outVar); 
            else
              sprintf(token, "%d", (int)outVar);
          }
          if(pcTheTabRet == 6)
            strcpy(token, "NaN");
        }else if(edCN)
          icolAcol(token, colN, prevColN, argc, argv, delPlc);
        if(dcols(argc, argv, colN, delPlc)){
          dontPtNextChar = true;
          if(colN == 1)
            dontPtNextDel = true;
        }
        if(colN && !strchr(token, '\n') && !dontPtNextChar){
          if(colN > 1 && !dontPtNextDel)
            printf("%c", argv[delPlc][0]);
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
      return 9; //if there are two or more rows with different numbers of columns
    }
  }
  arow(argc, argv, delPlc, colN);
  return 0;
}

