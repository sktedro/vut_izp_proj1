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
** 10 using 'rows' selection command with rseq, rsum, ravg, rmin, rmax or rcount
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

bool replaceDelims(char buffer[mxRwL], char *del, int rowN, int *firstColN){
  int dels = 1;
  for(int i = 0; buffer[i]; i++){
    for(int j = 0; del[j]; j++){
      if(buffer[i] == del[j]){
        buffer[i] = del[0];
        dels++;
        if(rowN == 1)
          (*firstColN)++;
      }
    }
  }
  if(rowN > 1 && dels != *firstColN){
    fprintf(stderr, "NOT ALL ROWS HAVE THE SAME NUMBER OF COLUMNS! THE PROGRAM WILL NOW EXIT.\n");
    return false;
  }
  return true;
}

//return false on error
bool isCmd(int argc, char *argv[], int delPlc, int *j, int cmdAsN[], char cmdArr[][mxCL], int *cmdN, char CA[][mxSL]){
  char exceptions[][mxCL] = {"cset", "beginswith", "contains"};//the only ones that can have a string as an argument
  int numOfExceptions = 3;
  if(*j == delPlc)
    return true;
    /*if(*j > 2){
      for(int i = 0; i < 3; i++){
        if(!strcmp(argv[*j - 2], exceptions[i])){
          *j += (cmdAsN[i]-1);
          return 0;
        }
      }
    }*/

  //dont scan the command, if there's an exception two places before
  //(the command could be an string argument for a different command, or a delimiter string)
  for(int i = 0; *j < argc && cmdArr[i][0]; i++){
    if(!strcmp(argv[*j], cmdArr[i])){
      if((*j + cmdAsN[i]) >= argc || (delPlc > *j && delPlc <= (*j + cmdAsN[i] + 1))){
          fprintf(stderr, "THERE ARE NOT ENOUGH ARGUMENTS FOR YOUR COMMAND '%s'! THE PROGRAM WILL NOW EXIT.\n", cmdArr[i]);
          return false;
      }
      strcpy(CA[0], cmdArr[i]);
      for(int l = 0; l < cmdAsN[i]; l++){
        bool exception = false;
        for(int m = 0; m < numOfExceptions; m++)
          if(!strcmp(cmdArr[i], exceptions[m]) && l == 1) //all exceptions can only have string as the second arg
            exception = true;
            //l = cmdAsN[i];
        if(argc > *j + l){
          char *strtolptr;
          int val = strtol(argv[*j + l + 1], &strtolptr, 10);
          bool BArg = (!strcmp(argv[*j], "rseq") && l == 3) || (!strcmp(argv[*j], "cseq") && l == 2);//if it is the B argument (which can be a negative or decimal number
          bool canBeADash = !strcmp(argv[*j], "rows") || (!strcmp(argv[*j], "rseq") && l == 2);
          if(!exception){
            if((strtolptr[0] && strtolptr[0] != '-' && strtolptr[0] != '.') //the argument can contain - or . in certain cases
                || (!BArg && strtolptr[0] == '.') //if it is not the B argument, but contains '.'
                || (!canBeADash && strtolptr[0] == '-') //if it cannot be a dash, but it is
                || (!BArg && !strtolptr[0] && val <= 0)){ //if it is not the B argument, but the arg value is <= 0
              fprintf(stderr, "ARGUMENT/ARGUMENTS FOLLOWING YOUR COMMAND '%s' IS/ARE SUPPOSED TO BE A NON-ZERO NUMBER! THE PROGRAM WILL NOW EXIT.\n", cmdArr[i]);
              return false;
            }
          }
          exception = false;
          strcpy(CA[l+1], argv[*j + l + 1]);
        }
      }
      *cmdN += 1;
      *j += cmdAsN[i];
      return true;
    }
  }
  return true;
}

bool cpSpMvFn(char buffer[mxRwL], char token[2*mxSL], char *del, int pcCA2, bool move, bool reverseAs){
  int actCol = 1;
  for(int i = 0; i < mxRwL; i++){
    if(buffer[i] == del[0])
      actCol++;
    if(actCol == pcCA2){
      int j = 0;
      i++;
      if(buffer[i+j] == del[0]){
        token[0] = '\0';
        return true;
      }  
      char theOtherToken[2*mxSL];
      while((i + j + 1) < (int)strlen(buffer) && buffer[i+j] != del[0] && buffer[i+j] != '\0'){
        theOtherToken[j] = buffer[i+j];
        j++;
      }
      theOtherToken[j] = '\0';
      if(move && reverseAs){
        strncat(theOtherToken, del, 1);
        strcat(theOtherToken, token);
        strcpy(token, theOtherToken);
        return true;
      }
      strcpy(token, theOtherToken);
      return true;
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
    if(move && !reverseAs)
      return 2;
    if((copy && !reverseAs) || cpSpMvFn(buffer, token, del, pcCA2, move, reverseAs))
      return 1;
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

bool doIEdFn(char slC[mxCAsN][mxSL], char buffer[], char *del, int rowN/*, int colN*/){
  char *strtolptr;
  int arg1 = strtol(slC[1], &strtolptr, 10);
  if(!slC[0][0])
    return true;
  else if(!strcmp(slC[0], "rows")){
    char *strtolptr2;
    int arg2 = strtol(slC[2], &strtolptr2, 10);
    if(slC[2][0] == '-'){
      if(slC[1][0] == '-'){ 
        if(rowN < 0)
          return true;
      }else if(rowN >= arg1 || (rowN < 0 && -rowN >= arg1))
        return true;
    }else if(arg1 > arg2){
      swapArgs(&arg1, &arg2);
    }else{
      if(rowN < 0)
        rowN = -rowN;      
      if(rowN >= arg1 && rowN <= arg2)
        return true;
    }
  }else if(!strcmp(slC[0], "beginswith") || !strcmp(slC[0], "contains")){
    char tempToken[mxSL];
    int tempColN = 1;
    int j = 0;
    for(int i = 0; buffer[i]; i++){
      if(buffer[i] == del[0] || buffer[i] == '\n')
        tempColN++;
      else if(tempColN == arg1){
        tempToken[j] = buffer[i];
        tempToken[++j] = '\0'; //increment j and write terminating null after the last char
      }
      if(tempColN > arg1 || buffer[i] == '\n')
        break;
    }
    if(!strcmp(slC[0], "beginswith")){
      bool doIEd = true;
      for(size_t i = 0; i < strlen(slC[2]); i++)
        if(tempToken[i] != slC[2][i])
          doIEd = false;
      return doIEd;
    }else if(!strcmp(slC[0], "contains") && strstr(tempToken, slC[2]) != NULL)
      return true;
  }
  return false;
}

int cSAMMCFn(double *cSAMMCVal, char buffer[mxRwL], char *del, char pcC[mxCAsN][mxSL], int colN){
  char cs[][mxCL] = {"csum", "cavg", "cmin", "cmax", "ccount"}; //commands
  int csNum = 5;
  double outVal = 0;
  char *strtolptr;
  for(int i = 0; i < csNum; i++){
    if(!strcmp(pcC[0], cs[i]) && strtol(pcC[1], &strtolptr, 10)){
      bool retNULL = true; //If value of this bool isnt changed during this cycle and retNULLHelper will be...
      bool retNULLHelper = false; //...set to true as well, "NaN" should be written to the cell instead of a number
      int arg1 = strtol(pcC[1], &strtolptr, 10);
      if(arg1 == colN){
        char *strtolptr2;
        char *strtolptr3;
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
        char tempTok[2*mxSL];
        for(int j = 0; j < (int)strlen(buffer); j++){
          if(buffer[j] == del[0] || buffer[j] == '\n'){ //if we approached a delimiter or a line break, it means an end of a cell
            if(actCol >= arg2 && actCol <= arg3){
              tempTok[k] = '\0';
              char *strtodptr;
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
  }
  return 1; //if no changes have been made
}

int cseq(int colN, char pcC[mxCAsN][mxSL], double *cSAMMCVal){
  int arg1 = atoi(pcC[1]);
  int arg2 = atoi(pcC[2]);
  double arg3 = atof(pcC[3]);
  swapArgs(&arg1, &arg2); //if arg1 > arg2, swap them
  if(colN == arg1)
    *cSAMMCVal = arg3;
  else if(colN > arg1 && colN <= arg2)
    *cSAMMCVal += 1;
  else
    return 1; //no changes
  return 4; //write the value into the table
}

int rseq(char pcC[mxCAsN][mxSL], int rowN, int colN, double *rSAMMCVal, bool *rseqInit){
  char *strtolptr[3];
  int arg[3];
  for(int i = 0; i < 3; i++)
    arg[i] = strtol(pcC[i+1], &strtolptr[i], 10);
  double arg4;
  arg4 = atof(pcC[4]);
  if(rowN < 0)
    rowN = 0 - rowN;
  if(strtolptr[2][0] == '-')
    arg[2] = rowN + 1; //if there is a - as the third arg, rseq will work for every line from arg2
  swapArgs(&arg[1], &arg[2]); //if arg1 > arg2, swap them
  if(colN == arg[0] && rowN >= arg[1] && rowN <= arg[2]){
    if(*rseqInit == false){
      *rseqInit = true;
      *rSAMMCVal = arg4; 
    }
    else if(rowN > arg[1] && rowN <= arg[2])
      *rSAMMCVal += 1;
    return 5; //write the value
  }
  return 1; //no changes
}

int rSAMMCFn(char token[2*mxSL], char pcC[mxCAsN][mxSL], int colN, int rowN, double *rSAMMCVal, int *rSAMMCCt){
  char cs[][mxCL] = {"rsum", "ravg", "rmin", "rmax", "rcount"}; //commands
  int csNum = 5;
  bool edit = false;
  for(int i = 0; i < csNum; i++)
    if(!strcmp(pcC[0], cs[i]))
      edit = true;
  if(!edit)
    return 1;
  if(rowN < 0)
    rowN = 0 - rowN;
  int arg1 = atoi(pcC[1]);
  if(colN == arg1){
    char *strtodptr;
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
        (*rSAMMCCt)++;
      }else if(!strcmp(pcC[0], "rmin") || !strcmp(pcC[0], "rmax")){
        if(*rSAMMCCt == 0){
          *rSAMMCVal = tokVal;
          (*rSAMMCCt)++;
        }else if(!strcmp(pcC[0], "rmin") && tokVal < *rSAMMCVal)
          *rSAMMCVal = tokVal;
        else if(!strcmp(pcC[0], "rmax") && tokVal > *rSAMMCVal)
          *rSAMMCVal = tokVal;
      }else if(!strcmp(pcC[0], "rcount"))
        (*rSAMMCVal)++;
    }
    if(rowN == arg3 + 1){ //if the actual row is the one to write the output to
      if(!strcmp(pcC[0], "ravg"))
        *rSAMMCVal /= (*rSAMMCCt);
      if(*rSAMMCCt == 0 && (!strcmp(pcC[0], "rmin") || !strcmp(pcC[0], "rmax") || !strcmp(pcC[0], "ravg")))
        return 6; //for commands rmin, rmax and ravg - if none of the processed cells was used to compute the output
      return 5;
    }
  }
  return 1;
}

bool pcNumsAndChars(char pcC[mxCAsN][mxSL], char token[2*mxSL]){
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
    bool isNum = true;
    for(size_t i = 0; i < strlen(token); i++)
      if((token[i] < '0' || token[i] > '9') && token[i] != '.')
        isNum = false;
    if(isNum)
      sprintf(token, "%d", (int)atol(token));
  }else
    return false;
  return true;
}

int pcTheTab(char buffer[], char *del, char token[2*mxSL], char tempToken[2*mxSL], int rowN, int colN, 
    char pcC[mxCAsN][mxSL], char slC[mxCAsN][mxSL], double *cSAMMCVal, double *rSAMMCVal, int *rSAMMCCt, bool *rseqInit){
  int doIEdFnRet = doIEdFn(slC, buffer, del, rowN/*, colN*/);
  if(doIEdFnRet != 1 && pcC[0][0] == 'r' && strcmp(pcC[0], "rseq")
      && (rowN == (atoi(pcC[3]) + 1) || -rowN == (atoi(pcC[3]) + 1)))
    doIEdFnRet = 1; //for rsum to rcount, if the actual row is M+1 (M as the arg), we edit, anyways. 
  else if(!strcmp(pcC[0], "rseq") && (rowN == (atoi(pcC[3]) + 1) || -rowN == (atoi(pcC[3]) + 1)) && (rowN > 1 || rowN < 1))
    doIEdFnRet = 1;
  if(doIEdFnRet == 1){
    int cSAMMCFnRet = cSAMMCFn(cSAMMCVal, buffer, del, pcC, colN);
    int rSAMMCFnRet = rSAMMCFn(token, pcC, colN, rowN, rSAMMCVal, rSAMMCCt);
    if(cSAMMCFnRet < 0 || cSAMMCFnRet > 1)
      return cSAMMCFnRet;
    if(rSAMMCFnRet < 0 || rSAMMCFnRet > 1)
      return rSAMMCFnRet; //err
    if(!strcmp(pcC[0], "cseq"))
      return cseq(colN, pcC, cSAMMCVal);
    if(!strcmp(pcC[0], "rseq"))
      return rseq(pcC, rowN, colN, rSAMMCVal, rseqInit);
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

bool skipEdC(char *argv[], int i){
  if((i > 1 && !strcmp(argv[i-1], "-d")) || (i > 2 && !strcmp(argv[i-2], "cset")))
    return true;
  return false;
}

//detecting if the user is trying to delete same column twice
bool delSameCol(int argc, char *argv[]){
  int deletedCols[mxRwL];
  int j;
  for(int i = 1; i < argc; i++){
    if(!skipEdC(argv, i)){
      if(!strcmp(argv[i], "dcol") && i + 1 < argc){
        deletedCols[j++] = atoi(argv[i+1]);
      }else if(!strcmp(argv[i], "dcols") && i + 2 < argc){
        int arg1 = atoi(argv[i+1]);
        int arg2 = atoi(argv[i+2]);
        swapArgs(&arg1, &arg2);
          for(int k = 0; (k <= (arg2 - arg1)) && (arg2 + j < mxRwL); k++)
            deletedCols[j++] = arg1 + k;
      }
      deletedCols[j+1] = 0;
    }
  }
  for(int i = 0; deletedCols[i]; i++)
    for(int k = i+1; deletedCols[k]; k++)
      if(deletedCols[i] == deletedCols[k])
        return true;
  return false;
}

void icolAcol(int colN, int firstColN, int argc, char *argv[], char *del, bool acol){
  int deletedColumns = 0;
  int colsToPrint = 0;
  for(int i = 1; i < argc; i++){
    if(!skipEdC(argv, i)){
      if(!strcmp(argv[i], "dcol"))
        deletedColumns++;
      else if(!strcmp(argv[i], "dcols") && atoi(argv[i+2]) <= firstColN)
        deletedColumns += (atoi(argv[i+2]) - atoi(argv[i+1]) + 1); 
    }
  }
  if(!acol){ //(if icol is called)
    bool acolPresent = false;
    for(int i = 1; i < argc; i++){
      if(!skipEdC(argv, i)){
        if(!strcmp(argv[i], "icol") && colN == atoi(argv[i+1]) && firstColN >= atoi(argv[i+1]))
          colsToPrint++;
        else if(!strcmp(argv[i], "acol"))
          acolPresent = true;
      }
    }
    if(deletedColumns >= firstColN && !acolPresent)
      colsToPrint--;
    for(int i = 0; i < colsToPrint; i++)
      printf("%c", del[0]);
  }
  if(acol){
    for(int i = 1; i < argc; i++)
      if(!skipEdC(argv, i))
        if(!strcmp(argv[i], "acol"))
          colsToPrint++;
    if(deletedColumns >= firstColN)
      colsToPrint--;
    for(int i = 0; i < colsToPrint; i++)
      if(colN == firstColN)
        printf("%c", del[0]);
  }
}

void irowArow(int argc, char *argv[], int delPlc, char *del, int rowN, int firstColN, bool arow){
  int deletedColumns = 0;
  int insertedColumns = 0;
  for(int i = 0; i < argc; i++){
    if(!skipEdC(argv, i)){
      if((!strcmp(argv[i], "icol") && firstColN >= atoi(argv[i+1])) || !strcmp(argv[i], "acol"))
        insertedColumns++;
      else if(!strcmp(argv[i], "dcol"))
        deletedColumns++;
      else if(!strcmp(argv[i], "dcols") && atoi(argv[i+2]) <= firstColN)
        deletedColumns += (atoi(argv[i+2]) - atoi(argv[i+1]) + 1); 
    }
  }
  int colsToPrint = firstColN + insertedColumns - deletedColumns - 1;
  if(deletedColumns >= firstColN)
    colsToPrint = insertedColumns - 1;
  for(int i = 1; i < argc; i++){
    if(!skipEdC(argv, i)){
      if(i != delPlc && !strcmp(argv[i], "irow") && rowN == atoi(argv[i+1])){
        for(int j = 0; j < colsToPrint; j++)
            printf("%c", del[0]);
        printf("\n");
      }
      if(i != delPlc && !strcmp(argv[i], "arow") && arow == true){
        for(int j = 0; j < colsToPrint; j++)
            printf("%c", del[0]);
        printf("\n");
      }
    }
  }
}

bool drows(int argc, char *argv[],/* int delPlc, */int rowN){
  if(rowN < 0)
    rowN = -rowN;
  for(int i = 1; i < argc; i++){
    if(!skipEdC(argv, i)){
      /*if(i == delPlc)
        continue;*/
      if((!strcmp(argv[i], "drow") && rowN == atoi(argv[i+1]))
        || (!strcmp(argv[i], "drows") && rowN >= atoi(argv[i+1]) && rowN <= atoi(argv[i+2])))
        return 1;
    }
  }
  return 0;
}

bool dcolDcols(int argc, char *argv[], int colN, int firstColN/*, int delPlc*/){
  for(int i = 0; i < argc; i++){
    if(!skipEdC(argv, i)){
      //if(i != delPlc){
        if(!strcmp(argv[i], "dcol") && colN == atoi(argv[i+1]))
            return true;
        if(!strcmp(argv[i], "dcols") && firstColN >= atoi(argv[i+2]) 
            && colN >= atoi(argv[i+1]) && colN <= atoi(argv[i+2]))
          return true;
      //}
    }
  }
  return false;
}

//this function searches for processing and selection commands. If found, writes them into pcC and slC
/*bool getSlAndPcC(int argc, char *argv[], char pcC[mxCAsN][mxSL], char pcCArr[][mxCL],
    int pcCAN[], char slC[mxCAsN][mxSL], char slCArr[][mxCL], int slCN){
  for(int i = 1; i < argc; i++)
    for(int j = 0; pcCAN[j]; j++) //pcCANCt is array with a number of args for each cmd. Iterations = it's length
      if(!strcmp(argv[i], pcCArr[j])){
        strcpy(pcC[0], argv[i]);
        for(int k = 0; k < (pcCAN[j] + 1); k++) //pcCAN[j] is a number saying how many arguments should there be for the cmd
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
}*/

//checks every single argument typed by the user and gets pc and sel cmds, if there are some
int getArgs(int argc, char *argv[], int delPlc, int *edCN, int *pcCN, int *slCN, char pcC[mxCAsN][mxSL], char slC[3+1][mxSL]){
  char edCArr[][mxCL] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"};
  int edCAN[] = {1, 0, 1, 2, 1, 0, 1, 2};

  char pcCArr[][mxCL] = {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move", 
      "csum", "cavg", "cmin", "cmax", "ccount", "cseq", "rseq", "rsum", "ravg", "rmin", "rmax", "rcount"};
  int pcCAN[] = {2, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 0}; //0 is there just to know the end

  char slCArr[][mxCL] =  {"rows", "beginswith", "contains"};
  int slCAN[] = {2, 2, 2};

  char edC[3][mxSL];

  for(int i = 1; i < argc; i++){
    //if(i == delPlc)
      //i++; //if the program approaches delimiters, it skips them
    bool isCmdPcRet = isCmd(argc, argv, delPlc, &i, pcCAN, pcCArr, pcCN, pcC);
    bool isCmdSlRet = isCmd(argc, argv, delPlc, &i, slCAN, slCArr, slCN, slC);
    bool isCmdEdRet = true;
    if(!(*pcCN) || !(*slCN))
      isCmdEdRet = isCmd(argc, argv, delPlc, &i, edCAN, edCArr, edCN, edC);
    if(isCmdEdRet == false || isCmdPcRet == false || isCmdSlRet == false)
      return -3;
    if((*pcCN || *slCN) && *edCN){
      fprintf(stderr, "YOU CANNOT EDIT AND PROCESS THE TABLE IN ONE EXECUTION! THE PROGRAM WILL NOW EXIT.\n");
      return -7; //In case of there being a table pcessing command and also a table eding command 
    }
  }
  if(*edCN && delSameCol(argc, argv)){
    fprintf(stderr, "YOU CANNOT DELETE ONE COLUMN MULTIPLE TIMES! THE PROGRAM WILL NOW EXIT. \n");
    return -11;
  }
  if(*pcCN > 1 || *slCN > 1){
    fprintf(stderr, "ONLY ONE PROCESSING/SELECTION COMMAND IS ACCEPTABLE (OR NONE)! THE PROGRAM WILL NOW EXIT.\n");
    return -6;
  }
  if(*pcCN && !strcmp(slC[0], "rows") && pcC[0][0] == 'r'){
    fprintf(stderr, "IT IS FORBIDDEN TO USE THE 'rows' SELECTION COMMAND ALONG WITH YOUR '%s' COMMAND! THE PROGRAM WILL NOW EXIT.\n", pcC[0]);
    return -10;
  }
  if(*pcCN != 0 && *edCN != 0) //if there is nothing to be done to the table
    return 0;
  if(*edCN > 0)
    return 1;
  if(*pcCN == 1)
    return 2;
  return 0;
}

int main(int argc, char *argv[]){
  int delPlc = getDelPlace(argc, argv); //Get info about where the deliter chars are in argv
  char *del = " ";
  if(delPlc < 0) //if -1 is returned, one of the arguments is too long. For -2 there are too many arguments
    return -delPlc; //getDelPlace will return -1 or -2, so from main, we return 1 or 2
  if(delPlc > 0)
    del = argv[delPlc]; //if there are arguments specified, del will point to delimiters in argv
  int edCN = 0; //amount of editing commands
  int pcCN = 0; //amount of processing commands
  int slCN = 0;
  char pcC[mxCAsN][mxSL]; //storing the name and arguments of a processing command
  char slC[mxCAsN][mxSL];
  char prevBuffer[mxRwL]; //necessary because I don't know that a line is the last one, until I load it into a buffer
  char buffer[mxRwL]; //lines will be temporarily saved here (one at a time) 
  char token[2*mxSL]; //generally, cells will be temporarily saved here (one at a time). 
  //in some functions, we need double the size, to save two tokens and a del simultaneously
  int rowN = 0; //keeping track of which line is the program actually pcessing (negative number for the last row)
  int colN = 0; //keeping track of which column is the program actually pcessing
  int firstColN = 1; //keeping track of total columns of the first line
  int tempRet = getArgs(argc, argv, delPlc, &edCN, &pcCN, &slCN, pcC, slC);
  if(tempRet < 0)
    return -tempRet;
  if(tempRet == 0){//if there are no table editing or processing cmds
    while(fgets(buffer, mxRwL, stdin)){
      if(!replaceDelims(buffer, del, ++rowN, &firstColN))
        return 9;
      printf("%s", buffer);
    }
    return 0;
  }
  int finish = 0; //gets used when fgets returns EOF and we need to process the last line
  double rSAMMCVal = 0;
  int rSAMMCCt = 0;
  bool rseqInit = false;
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
    if(!replaceDelims(buffer, del, rowN, &firstColN)) //returns false if there are different amounts of columns
      return 9;
    if(edCN)
      irowArow(argc, argv, delPlc, del, rowN, firstColN, 0); // the 0 means it's irow, not arow
    if(edCN && drows(argc, argv/*, delPlc*/, rowN))
      continue;
    colN = 0;
    int l = 0;
    bool dontPtNextDel = false;
    double cSAMMCVal = 0; //column SumAvgMinMaxCount
    char tempToken[2*mxSL];
    for(size_t k = 0; k < strlen(buffer); k++){
      bool dontPtNextChar = false;
      if(buffer[k] == del[0] && (k == 0 || buffer[k-1] == del[0])) //if the first char of a line is a delim or previous char was a delim 
        token[l++] = 0; //write terminal null as the last tokens char and increase l by one
      if(buffer[k] != del[0] && buffer[k] != '\n'){ //if a char isn't a delim and neither a line break
        token[l] = buffer[k]; //copy a char from the buffer
        token[++l] = 0; //and also increase the l by one, and finally, put terminal null after the character
      }else{
        colN++;
        if(pcCN == 1){ //if a table processing command was inputed by the user
          int pcTheTabRet = pcTheTab(buffer, del, token, tempToken, rowN, colN, pcC, slC, &cSAMMCVal, &rSAMMCVal, &rSAMMCCt, &rseqInit);
          if(pcTheTabRet < 0){
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
            if(tempVar > 0)
              while(tempVar >= 1)
                tempVar--;
            else if(tempVar < 0)
              while(tempVar <= -1)
                tempVar++;
            if(tempVar > -1 && tempVar < 1)
              sprintf(token, "%g", outVar); 
            else
              sprintf(token, "%d", (int)outVar);
          }
          if(pcTheTabRet == 6)
            strcpy(token, "NaN");
        }else if(edCN)
          icolAcol(colN, firstColN, argc, argv, del, 0); //0 indicating, this is supposed to run icol
        if(edCN && dcolDcols(argc, argv, colN, firstColN/*, delPlc*/)){
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
        if(edCN)
          icolAcol(colN, firstColN, argc, argv, del, 1); //1 indicating, this is supposed to run acol
        l = 0;
      }
    }
    printf("\n");
    /*if(firstColN != colN && rowN >= 0){
      fprintf(stderr, "NOT ALL ROWS HAVE THE SAME NUMBER OF COLUMNS! THE PROGRAM WILL NOW EXIT.\n");
      return 9; //if there are two or more rows with different numbers of columns
    }*/
  }
  if(edCN)
    irowArow(argc, argv, delPlc, del, rowN, firstColN, 1); //the one means it's arow, not irow
  return 0;
}

