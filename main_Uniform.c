/*
** The table eding commands entered by the user are not executed in the order 
**   in which the user entered them.
**
** If a command requires some argument to be a lower number than the next one,
** generally, it swaps them - eg. dcols 3 1 will be executed like dcols 1 3
**
** Return values from main:
  ** 0 everything went well
  ** 1 one of the arguments is too long
  ** 2 there are too many arguments entered
  ** 3 there are not enough arguments for a command or an argument 
  **   is supposed to be a number but is not
  ** 4 an argument is an unacceptable number
  ** 5 problem with a selection command 
  ** 6 more than one table processing command or selection command was entered
  ** 7 one or more table processing or selection commands and one or more table 
  **   editing commands were entered simultaneously
  ** 8 one or more rows of the table exceed maximum row size (10KiB)
  ** 9 not all rows of the table have the same amount of columns
  ** 10 using 'rows' selection command with rseq, rsum, ravg, rmin, rmax or rcount
**
**
** Variables:
  ** S = string
  ** R = row
  ** C = command
  ** A = argument
  ** L = length
  ** N = number
  **
  ** pcC = processing command (arguments are stored with it)
  ** pcCN = number of processing commands entered by user
  ** pcCA = a single argument of a processing command 
  ** pcCAN = array of numbers of requiered arguments for processing commands
  ** pcCArr = array of names of defined processing commands
  **
  ** buffer = a single line
  ** tok = token, a table cell
  ** ed = eding
  ** pc = pcessing
  ** sl = slection
  ** col = column
  ** del = delimiter
  ** plc = place
  ** ct = count
  ** ret = returned value
  ** fn = function
  ** dup = duplicate
  ** deld = deleted
  ** insd = inserted
**
*/


#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAXRL 10242 //max length of a row is 10KiB
#define MAXSL 101   //max cell (string) size
#define MAXC 101    //max number of arguments in one run of the program
#define MAXAL 101   //max execution argument length
#define MAXCL 11    //length of the longest command
#define MAXCA 6     //array size to store a command with all its arguments

/*
** Misc functions
*/

//Swaps two arguments if one is bigger than the other
void swapArgs(int *arg1, int *arg2){ 
  if(*arg1 > *arg2){
    int tempVal = *arg1;
    *arg1 = *arg2;
    *arg2 = tempVal;
  }
}

//If there is "-d" one argument before, it means this argument is a delimiter. In case
//of it being also a name of a command
//Same goes with commands cset, beginswith and contains, if they are two steps back
bool skipEdC(char *argv[], int i){
  if(i > 1 && !strcmp(argv[i-1], "-d"))
    return true;
  if(i > 2)
    if(!strcmp(argv[i-2], "cset") 
      || !strcmp(argv[i-2], "beginswith") 
      || !strcmp(argv[i-2], "contains"))
    return true;
  return false;
}

//Detecting if a user is trying to delete the same column twice (or more)
bool dupDcol(int argc, char *argv[]){
  int deldCols[MAXRL];
  //Numbers of deleted columns will be saved to this array
  int j;
  for(int i = 1; i < argc; i++){
    if(!skipEdC(argv, i)){
      if(!strcmp(argv[i], "dcol") && i + 1 < argc){
        deldCols[j++] = atoi(argv[i+1]);
      }else if(!strcmp(argv[i], "dcols") && i + 2 < argc){
        int arg1 = atoi(argv[i+1]);
        int arg2 = atoi(argv[i+2]);
        swapArgs(&arg1, &arg2);  
        //Swap the values, if the first argument is greater
          for(int k = 0; (k <= (arg2 - arg1)) && (arg2 + j < MAXRL); k++)
            deldCols[j++] = arg1 + k;
      }
      deldCols[j+1] = 0;
    }
  }
  for(int i = 0; deldCols[i]; i++)
    for(int k = i+1; deldCols[k]; k++)
      if(deldCols[i] == deldCols[k])
        return true;
  return false;
}

/*
** Working with delimiters
*/

//Searches for -d and returns a number, at which place the delims are in argv
//Also, check if there are too many arguments or one of them is too long
int getDel(int argc, char *argv[]){
  if(argc > MAXC){
    fprintf(stderr, "YOU ENTERED TOO MANY ARGUMENTS! THE MAXIMUM AMOUNT IS 100! THE PROGRAM WILL NOW EXIT.\n");
    return -2;
  }
  for(int i = 1; i < (argc - 1); i++){
    for(int j = 0; argv[i][j]; j++)
      if(j >= MAXAL){
        fprintf(stderr, "ONE OF YOUR ARGUMENTS IS TOO LONG! THE MAXIMUM LENGTH IS 100! THE PROGRAM WILL NOW EXIT.\n"); 
        return -1;
      }
    if(strstr(argv[i], "-d"))
      return (i + 1); //i is the place of -d, i+1 is the place of delimiters
  }
  return 0;
}

//Takes a single row and replaces all delimiters with the main one (space, or
//if delimiters were specified, the first delimiter)
//Also, checks if the row has the same amount of columns as the first row
bool replaceDels(char buffer[MAXRL], char *del, int row, int *firstColN){
  int dels = 1; //Used to count delimiters in the row that's actually being processed
  for(int i = 0; buffer[i]; i++){
    for(int j = 0; del[j]; j++){
      if(buffer[i] == del[j]){
        buffer[i] = del[0];
        dels++; 
        if(row == 1)
          (*firstColN)++;
      }
    }
  }
  if(row > 1 && dels != *firstColN){
    fprintf(stderr, "NOT ALL ROWS HAVE THE SAME NUMBER OF COLUMNS! THE PROGRAM WILL NOW EXIT.\n");
    return false;
  }
  return true;
}

/*
** Checking arguments for validity and saving processing and selection commands
** (and their arguments) to an array (if there are some)
*/

//This is a sub-function of getArgs()
//getArgs() functions calls this one to find out, if there is a command on a
//certain place in *argv[]. If there is, it will be saved, along with it's
//arguments, to an array (which is passed to this function)
//isCmd() is called up to 3 times for each execution argument (first to check
//for table processing, then selection, and finally editing commands)
int isCmd(int argc, char *argv[], int delPlc, int *j, int cmdAsN[], 
    char cmdArr[][MAXCL], int *cmdN, char CA[][MAXSL]){
  char exceptions[][MAXCL] = {"cset", "beginswith", "contains"};
  int numOfExceptions = 3;
  //The only ones that can have a string as an argument
  bool foundCmd = false;
  if(*j == delPlc)
    return 1;
  for(int i = 0; *j < argc && cmdArr[i][0]; i++){
    if(!strcmp(argv[*j], cmdArr[i])){
      if((*j + cmdAsN[i]) >= argc 
          || (delPlc > *j && delPlc <= (*j + cmdAsN[i] + 1))){
        //Err if: j + number of arguments for this command cannot exceed argc
        //Err if: There are delimiters, where a command argument should be
        fprintf(stderr, "THERE ARE NOT ENOUGH ARGUMENTS FOR YOUR COMMAND '%s'! THE PROGRAM WILL NOW EXIT.\n", cmdArr[i]);
        return 0;
      }
      foundCmd = true;
      strcpy(CA[0], cmdArr[i]);
      //Copy the command to the first place of CA (edCA, pcCA or slCA)
      for(int l = 0; l < cmdAsN[i]; l++){
        bool exception = false;
        for(int m = 0; m < numOfExceptions; m++)
          if(!strcmp(cmdArr[i], exceptions[m]) && l == 1)
            exception = true;
            //If this argument is the second argument of an exception,
            //then it doesn't have to be a number
        if(argc > *j + l){
          char *tolptr;
          int val = strtol(argv[*j + l + 1], &tolptr, 10);
          bool BArg = (!strcmp(argv[*j], "rseq") && l == 3) 
                   || (!strcmp(argv[*j], "cseq") && l == 2);
          //If it is the B argument (which can be a negative or decimal number)
          bool canBeADash = !strcmp(argv[*j], "rows") 
                        || (!strcmp(argv[*j], "rseq") && l == 2);
          //Command 'rows' is allowed to have - as both arguments, and 
          //'rseq' as the third argument
          if(!exception){
            if((tolptr[0] && tolptr[0] != '-' && tolptr[0] != '.') 
                || (!BArg && tolptr[0] == '.') 
                || (!canBeADash && tolptr[0] == '-') 
                || (!BArg && !tolptr[0] && val <= 0)){ 
              //The argument can contain - or . in certain cases
              //If it is not the B argument (must be an integer), but contains '.'
              //If it cannot be a dash , but it is
              //If it is not the B argument, but the arg value is <= 0
              fprintf(stderr, "ARGUMENT/ARGUMENTS FOLLOWING YOUR COMMAND '%s' IS/ARE SUPPOSED TO BE A NON-ZERO NUMBER! THE PROGRAM WILL NOW EXIT.\n", cmdArr[i]);
              return 0;
            }
          }
          exception = false;
          strcpy(CA[l+1], argv[*j + l + 1]);
          //Copy the argument to the CA array (edC, pcC or slC) 
        }
      }
      *cmdN += 1; 
      //Increases number of commands of a certain type by one
      *j += cmdAsN[i];
      //Skips checking execution arguments that are arguments to a command
      if(foundCmd)
        return 2;
      return 1;
    }
  }
  return 1;
}

//Checks every single argument entered by a user
//Saves processing and selection commands, if there are some
//Also, checks for errors
int getArgs(int argc, char *argv[], int delPlc, int *edCN, 
    int *pcCN, int *slCN, char pcC[MAXCA][MAXSL], char slC[MAXCA][MAXSL]){
  char edCArr[][MAXCL] = 
    {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"};
  int edCAN[] = {1, 0, 1, 2, 1, 0, 1, 2};
  //Names of editing commands and numbers saying, how many arguments should they have
  char pcCArr[][MAXCL] = 
    {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move", 
     "csum", "cavg", "cmin", "cmax", "ccount", "cseq", 
     "rseq", "rsum", "ravg", "rmin", "rmax", "rcount"};
  int pcCAN[] = {2, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 0};
  //Same for table processing commands...
  char slCArr[][MAXCL] =  {"rows", "beginswith", "contains"};
  int slCAN[] = {2, 2, 2};
  //...and row selection commands
  char edC[3][MAXSL];
  //Name and arguments of a table editing command will be saved here. This is
  //pretty much useless, because there can be tens of these commands entered
  for(int i = 1; i < argc; i++){
    int isCmdPcRet = isCmd(argc, argv, delPlc, &i, pcCAN, pcCArr, pcCN, pcC);
    if(isCmdPcRet == 2)
      continue;
    //If a table processing command was found, start over
    int isCmdSlRet = isCmd(argc, argv, delPlc, &i, slCAN, slCArr, slCN, slC);
    if(isCmdSlRet == 2)
      continue;
    //If a row selection command was found, start over
    int isCmdEdRet = true;
    if(!(*pcCN) || !(*slCN))
      isCmdEdRet = isCmd(argc, argv, delPlc, &i, edCAN, edCArr, edCN, edC);
    //Only search for table editing commands if there are no processing or
    //selection commands entered so far
    if(!isCmdEdRet|| !isCmdPcRet || !isCmdSlRet )
      return -3;
    if((*pcCN || *slCN) && *edCN){
      fprintf(stderr, "YOU CANNOT EDIT AND PROCESS THE TABLE IN ONE EXECUTION! THE PROGRAM WILL NOW EXIT.\n");
      return -7; 
    }
  }
  if(*edCN && dupDcol(argc, argv)){
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
  if(*pcCN != 0 && *edCN != 0) 
    return 0;
    //If there is nothing to be done with the table
  if(*edCN > 0)
    return 1;
  if(*pcCN == 1)
    return 2;
  return 0;
}

/*
** Editing the table
*/

//A function inserting or appending a column
void icolAcol(int col, int firstColN, int argc, char *argv[], char *del, bool acol){
  //colsToPt--; command explaination:
    //We must take into consideration, that when there is not a single 
    //delimiter and neither a character, it can mean that there are no
    //columns or that there is one empty column)
    //This applies when all the table columns were deleted
  int deldCols = 0;
  int colsToPt = 0;
  for(int i = 1; i < argc; i++){
    if(!skipEdC(argv, i)){
      if(!strcmp(argv[i], "dcol"))
        deldCols++;
      else if(!strcmp(argv[i], "dcols"))
        deldCols += (atoi(argv[i+2]) - atoi(argv[i+1]) + 1); 
    }
  }
  //It is necessary to know how many columns were deleted, so the appended
  //column will have an appropriate number of columns
  if(!acol){ 
    //If icol is called
    int icolAcolHelper = 0;
    //If more than one column is added, this will be set to 1.
    //If set to one and all columns were deleted, prints at least one delim
    int firstIcol = 20000;
    //Number of the first column, before which a column is to be inserted, will
    //be written there (initial value has to be bigger than the max row length)
    for(int i = 1; i < argc; i++){
      if(!skipEdC(argv, i)){
        if(!strcmp(argv[i], "icol")){
          icolAcolHelper++;
          int arg1 = atoi(argv[i+1]);
          if(firstIcol > arg1)
            firstIcol = arg1;
          if(col == arg1 && firstColN >= arg1)
            colsToPt++;
        }else if(!strcmp(argv[i], "acol"))
          icolAcolHelper++;
      }
    }
    if(deldCols >= firstColN){
      //If all the columns were deleted...
      if(icolAcolHelper < 2)
        colsToPt = 0;
        //...and only one or no columns were inserted. Print no delimiter 
      else if(firstIcol == col && icolAcolHelper >= 2)
        colsToPt--;
        //...otherwise, print one less delimiter
    }
    for(int i = 0; i < colsToPt; i++)
      printf("%c", del[0]);
  }
  if(acol){
    //If acol is called
    int icolAcolHelper = 0;
    //Counts how many columns were inserted (if it is one or zero, and all the
    //columns were deleted, it prints no delimiter)
    for(int i = 1; i < argc; i++)
      if(!skipEdC(argv, i)){
        if(!strcmp(argv[i], "acol")){
          colsToPt++;
          icolAcolHelper++;
        }else if(!strcmp(argv[i], "icol"))
          icolAcolHelper++;
      }
    if(deldCols >= firstColN && icolAcolHelper < 2)
      colsToPt--;
    for(int i = 0; i < colsToPt; i++)
      if(col == firstColN)
        printf("%c", del[0]);
  }
}

//A function inserting or appending a row to the table
void irowArow(int argc, char *argv[], char *del, int row, int firstColN, bool arow){
  int deldCols = 0;
  int insdCols = 0;
  for(int i = 0; i < argc; i++){
    if(!skipEdC(argv, i)){
      if((!strcmp(argv[i], "icol") && firstColN >= atoi(argv[i+1])) 
          || !strcmp(argv[i], "acol"))
        insdCols++;
      //Keeping track of how many columns were inserted
      else if(!strcmp(argv[i], "dcol"))
        deldCols++;
      else if(!strcmp(argv[i], "dcols"))
        deldCols += (atoi(argv[i+2]) - atoi(argv[i+1]) + 1); 
    }
  }
  int colsToPt = firstColN + insdCols - deldCols - 1;
  if(deldCols >= firstColN)
    colsToPt = insdCols - 1;
    //If all the columns were deleted, only print the columns that were
    //inserted using a command (-1, because for 2 columns there is one delim)
  for(int i = 1; i < argc; i++){
    if(!skipEdC(argv, i)){
      if(!strcmp(argv[i], "irow") && arow == false){
        //If irow is called
        int arg = atoi(argv[i+1]);
        if(row < 0)
          row = 0 - row;
          //If this is the last row, it will have a negative value
        if(arg == row){
          for(int j = 0; j < colsToPt; j++)
              printf("%c", del[0]);
          printf("\n");
        }
      }
      if(!strcmp(argv[i], "arow") && arow == true){
        //If arow is called
        for(int j = 0; j < colsToPt; j++)
            printf("%c", del[0]);
        printf("\n");
      }
    }
  }
}

//Deleting columns. If a columns is to be deleted, next token (cell) along with
//next delimiter will not be printed
bool dcolDcols(int argc, char *argv[], int col){
  for(int i = 0; i < argc; i++){
    if(!skipEdC(argv, i)){
      if(!strcmp(argv[i], "dcol") 
          && col == atoi(argv[i+1]))
        return true;
      if(!strcmp(argv[i], "dcols")){
        int arg1 = atoi(argv[i+1]);
        int arg2 = atoi(argv[i+2]);
        swapArgs(&arg1, &arg2);   
        if(col >= arg1 && col <= arg2)
          return true;
      }
    }
  }
  return false;
}

//Deleting rows. If a row is to be deleted, the buffer will not be printed
bool drowDrows(int argc, char *argv[], int row){
  if(row < 0)
    row = -row;
    //If this is the last row, it will have a negative value
  for(int i = 1; i < argc; i++){
    if(!skipEdC(argv, i)){
      if(!strcmp(argv[i], "drow") && row == atoi(argv[i+1]))
        return 1;
      if(!strcmp(argv[i], "drows")){
        int arg1 = atoi(argv[i+1]);
        int arg2 = atoi(argv[i+2]);
        swapArgs(&arg1, &arg2);   
        if(row >= arg1 && row <= arg2)
          return 1;
      }
    }
  }
  return 0;
}

/*
** Table processing and row selection commands
*/

//Function that checks for row selection commands, and only returns true when a
//certain cell is supposed to be processed (based ona arguments for the
//provided row selection command
int doIEdFn(char slC[MAXCA][MAXSL], char buffer[], char *del, int row){
  char *tolptr;
  int arg1 = strtol(slC[1], &tolptr, 10);
  if(!slC[0][0])
    return 2;
  else if(!strcmp(slC[0], "rows")){
    char *tolptr2;
    int arg2 = strtol(slC[2], &tolptr2, 10);
    if(slC[2][0] == '-'){
      if(slC[1][0] == '-' && row < 0){
        return 2;
        //If 'rows - -' was entered, return true if the row number is negative
      }else if(slC[1][0] == '-' && row > 0){
        return 1;
      }else if(row >= arg1 || (row < 0 && -row >= arg1))
        return 2;
    }else if(slC[1][0] == '-' && slC[2][0] != '-'){
      fprintf(stderr, "YOUR 'rows' COMMAND IS CORRUPT. IF THE SECOND ARGUMENT IS NOT A '-', THE FIRST ARGUMENT HAS TO BE A NUMBER. THE TABLE WILL NOT BE PROCESSED.\n");
      return 0;
    }else{
      if(row < 0)
        row = -row;      
      swapArgs(&arg1, &arg2);
      if(row >= arg1 && row <= arg2)
        return 2;
    }
  }else if(!strcmp(slC[0], "beginswith") || !strcmp(slC[0], "contains")){
    char tempTok[MAXSL];
    int tempColN = 1;
    int j = 0;
    for(int i = 0; buffer[i]; i++){
      if(buffer[i] == del[0] || buffer[i] == '\n')
        tempColN++;
      else if(tempColN == arg1){
        tempTok[j] = buffer[i];
        tempTok[++j] = '\0'; 
        //Increment j and write a terminating null after the last char
      }
      if(tempColN > arg1 || buffer[i] == '\n')
        break;
    }
    //From buffer we get the cell, in which we check if it begins with a
    //certain string or contains it (based on the command entered)
    if(!strcmp(slC[0], "beginswith")){
      bool doIEd = true;
      int arg2Len = (int)strlen(slC[2]);
      for(int i = 0; i < arg2Len; i++)
        if(tempTok[i] != slC[2][i])
          doIEd = false;
          //If one or more of the characters is different
      if(doIEd == true)
        return 2;
    }else if(!strcmp(slC[0], "contains") && strstr(tempTok, slC[2]) != NULL)
      return 2;
  }
  return 1;
  //1 = Do not edit
  //2 = Do edit
  //0 = err
}

//Function, in which the characters and numbers are processed. 
//Includes commans: tolower, toupper, round and int
bool pcNumsAndChars(char pcC[MAXCA][MAXSL], char token[2*MAXSL]){
  if(!strcmp(pcC[0], "tolower")){
    for(size_t i = 0; i < strlen(token); i++)
      if(token[i] >= 'A' && token[i] <= 'Z')
        token[i] += 'a' - 'A';
  }else if(!strcmp(pcC[0], "toupper")){
    for(size_t i = 0; i < strlen(token); i++)
      if(token[i] >= 'a' && token[i] <= 'z')
        token[i] -= 'a' - 'A';
  }else if(!strcmp(pcC[0], "round")){
    char *todptr;
    double num = 0.5 + strtod(token, &todptr);
    int intN = num;
    if(!todptr[0])
      sprintf(token, "%d", intN);
  }else if(!strcmp(pcC[0], "int")){
    bool isNum = true;
    int tokLen = (int)strlen(token);
    for(int i = 0; i < tokLen; i++)
      if((token[i] < '0' || token[i] > '9') && token[i] != '.')
        isNum = false;
        //It is not a number if it contains other characters than 0-9 or '.'
    if(isNum)
      sprintf(token, "%d", (int)atol(token));
  }else
    return false;
  return true;
}

//Extension of the function cpSpMv
//Copies the desired cell before another cell (in case it is supposed to)
bool cpSpMvFn(char buffer[MAXRL], char token[2*MAXSL], 
    char *del, int pcCA2, bool move, bool reverseAs){
  int actCol = 1;
  for(int i = 0; i < MAXRL; i++){
    if(buffer[i] == del[0])
      actCol++;
    if(actCol == pcCA2){
      //If this is the column defined by the second argument
      int j = 0;
      i++;
      if(buffer[i+j] == del[0]){
        token[0] = '\0';
        return true;
        //If the cell is empty
      }  
      char theOtherToken[2*MAXSL];
      int bufferLen = (int)strlen(buffer);
      while((i + j + 1) < bufferLen 
          && buffer[i+j] != del[0] 
          && buffer[i+j] != '\0'){
        theOtherToken[j] = buffer[i+j];
        j++;
        //Copies a desired buffer into theOtherToken
      }
      theOtherToken[j] = '\0';
      if(move && reverseAs){
        strncat(theOtherToken, del, 1);
        strcat(theOtherToken, token);
        //If 'move' command was entered and the program is expected to move the
        //cell to the left, instead of to the right, it inserts the desired
        //cell before the next one (and inserts a delimiter between them)
      }
      strcpy(token, theOtherToken);
      return true;
    }
  }
  return 0;
}

//Copy, swap and move commands
int cpSpMv(char pcC[MAXCA][MAXSL], int pcCA1, int pcCA2, char *token, 
    char *tempTok, int col, char buffer[MAXRL], char *del){ 
  int maxC = 1;
  for(int i = 0; buffer[i]; i++)
    if(buffer[i] == del[0])
      maxC++;
  bool copy = !strcmp(pcC[0], "copy");
  bool move = !strcmp(pcC[0], "move");
  bool reverseAs = pcCA1 > pcCA2;
  //In case we are copying or moving backwards (right to left)
  if(maxC < pcCA2)
    return 0;
  if(move && pcCA1 + 1 == pcCA2)
    return 0;
  swapArgs(&pcCA1, &pcCA2);
  //We swap them anyways, so they will be easier to work with
  if(col == pcCA1){
    strcpy(tempTok, token);
    if(move && !reverseAs)
      return 2;
    if((copy && !reverseAs) || cpSpMvFn(buffer, token, del, pcCA2, move, reverseAs))
      return 1;
  }else{
    if(move && reverseAs){
      token[0] = '\0';
      return 3;
    }else if(move && !reverseAs){
      strncat(tempTok, del, 1);
      strcat(tempTok, token);
      //Inserting the cell that is supposed to be moved before the next cell
    }else if(copy && reverseAs)
      return 1;
    strcpy(token, tempTok);
  }
  return 1;
}

//Column processing commands function (for commands written two lines lower)
int colPcCsFn(double *colPcCsVal, char buffer[MAXRL], 
    char *del, char pcC[MAXCA][MAXSL], int col){
  char cs[][MAXCL] = {"csum", "cavg", "cmin", "cmax", "ccount"}; 
  int csNum = 5;
  double outVal = 0;
  char *tolptr;
  for(int i = 0; i < csNum; i++){
    if(!strcmp(pcC[0], cs[i]) && strtol(pcC[1], &tolptr, 10)){
      bool retNULL = true; 
      bool retNULLHelper = false; 
      //If value of this bool isnt changed during this cycle and retNULLHelper
      //will be set to true as well, "NaN" should be written to the cell
      int arg1 = strtol(pcC[1], &tolptr, 10);
      if(arg1 == col){
        char *tolptr2, *tolptr3;
        int arg2 = strtol(pcC[2], &tolptr2, 10);
        int arg3 = strtol(pcC[3], &tolptr3, 10);
        swapArgs(&arg2, &arg3);
        if((arg1 >= arg2 && arg1 <= arg3) || tolptr2[0] || tolptr3[0]){
          fprintf(stderr, "THE DESTINATION COLUMN FOR YOUR COMMAND '%s' CANNOT BE WITHIN THE RANGE OF THE OPERATION! THE PROGRAM WILL NOW EXIT.\n", pcC[0]);
          return -4;
        }
        bool init = false;
        int actCol = 1;
        int avgCt = 0;
        int k = 0;
        char tempTok[2*MAXSL];
        int bufferLen = (int)strlen(buffer);
        for(int j = 0; j < bufferLen; j++){
          if(buffer[j] == del[0] || buffer[j] == '\n'){ 
            //If we approached a delimiter or a line break, it means an end of a cell
            if(actCol >= arg2 && actCol <= arg3){
              //If the actual column is withing range of the command arguments
              tempTok[k] = '\0';
              char *todptr;
              double tokVal = strtod(tempTok, &todptr);
              if(!strcmp(pcC[0], "csum") && !todptr[0]){
                outVal = outVal + tokVal; 
              }else if(!strcmp(pcC[0], "cavg")){
                retNULLHelper = true;
                if(!todptr[0]){
                  outVal += tokVal;
                  avgCt++;
                  retNULL = false; 
                  //If at least one of processed cells is a number, there will
                  //be a number written to the table (instead of 'NaN')
                }
              }else if(!strcmp(pcC[0], "cmin") || !strcmp(pcC[0], "cmax")){
                retNULLHelper = true;
                if(!todptr[0]) 
                  retNULL = false;
                  //If at least one of processed cells is a number, there will
                  //be a number written to the table (instead of 'NaN')
                if(!init && !todptr[0]){ 
                  outVal = tokVal;
                  init = true;
                  //If this is the first cycle (!init) and there is a number in
                  //the column, copy it (it will be the starting number)
                }else if(!strcmp(pcC[0], "cmin") && outVal > tokVal && !todptr[0])
                  outVal = tokVal;
                else if(!strcmp(pcC[0], "cmax") && outVal < tokVal && !todptr[0])
                  outVal = tokVal;
              }else if(!strcmp(pcC[0], "ccount"))
                if(!todptr[0])
                  outVal++;
            }
            actCol++;
            k = 0;
          }
          else if(actCol >= arg2 && actCol <= arg3) 
            tempTok[k++] = buffer[j]; 
            //If the actual column is to be processed, keep writing characters
            //from the buffer until we reach the delim or line break (mind the k++)
        }
        if(!strcmp(pcC[0], "cavg"))
          outVal /= avgCt; 
          //Divide the sum by how many numbers were summed so we get an average
        *colPcCsVal = outVal; 
        if(retNULL && retNULLHelper)
          return 6; 
          //In case no number should be written into the cell, write "NaN" instead
          //In case of cmin, cmax or cavg, but there's not a single number to
          //be worked with
        return 4; 
        //Write the value into the cell 
      }
    }
  }
  return 1; 
  //If no changes have been made
}

//Simple enough. Writes a sequence of numbers into a column. The bigger the
//column number, the bigger the number written (increments by one)
int cseq(int col, char pcC[MAXCA][MAXSL], double *colPcCsVal){
  int arg1 = atoi(pcC[1]);
  int arg2 = atoi(pcC[2]);
  double arg3 = atof(pcC[3]);
  swapArgs(&arg1, &arg2);
  if(col == arg1)
    *colPcCsVal = arg3;
    //If this is the first run, we're starting with the B (third) argument
  else if(col > arg1 && col <= arg2)
    *colPcCsVal += 1;
    //For the rest of the row, we just increase by one for each column
  else
    return 1; 
    //No changes
  return 4; 
  //Write the value into the table
}

//Similar to cseq, but the incremented numbers will be printed to next rows,
//instead of next columns. Also, it is acceptable to put '-' as the third
//argument, indicating the numbers should be inserted for all rows after the
//row specified by the second argument
int rseq(char pcC[MAXCA][MAXSL], int row, int col, double *rowPcCsVal, bool *rseqInit){
  char *tolptr[3];
  int arg[3];
  for(int i = 0; i < 3; i++)
    arg[i] = strtol(pcC[i+1], &tolptr[i], 10);
  double arg4;
  arg4 = atof(pcC[4]);
  //Fourth argument can have decimal places
  if(row < 0)
    row = 0 - row;
  if(tolptr[2][0] == '-')
    arg[2] = row + 1; 
    //If there is a '-' as the third arg, rseq applies to all rows after the
    //row declared by the second argument
  swapArgs(&arg[1], &arg[2]); 
  if(col == arg[0] && row >= arg[1] && row <= arg[2]){
    if(*rseqInit == false){
      *rseqInit = true;
      *rowPcCsVal = arg4; 
      //If this is the first run, we start with the number in the 4th argument
    }
    else if(row > arg[1] && row <= arg[2])
      *rowPcCsVal += 1;
    return 5; 
    //Write the value
  }
  return 1; 
  //No changes
}

//Similar to colPcCsFn, but all the commands are to be executed within a
//column, but within a set of rows. The output value is inserted after the last
//row processed
int rowPcCsFn(char token[2*MAXSL], char pcC[MAXCA][MAXSL], int col, int row, 
    double *rowPcCsVal, int *rowPcCsCt){
  char cs[][MAXCL] = {"rsum", "ravg", "rmin", "rmax", "rcount"}; 
  int csNum = 5;
  bool edit = false;
  for(int i = 0; i < csNum; i++)
    if(!strcmp(pcC[0], cs[i]))
      edit = true;
  if(!edit)
    return 1;
  if(row < 0)
    row = 0 - row;
  int arg1 = atoi(pcC[1]);
  if(col == arg1){
    char *todptr;
    double tokVal = strtod(token, &todptr);
    int arg2 = atoi(pcC[2]);
    int arg3 = atoi(pcC[3]);
    swapArgs(&arg2, &arg3);
    //If the second argument is a higher number than the third, swap them
    if(row >= arg2 && row <= arg3){
      if(todptr[0])
        return 1;
        //If it is not a number
      if(!strcmp(pcC[0], "rsum")){
        *rowPcCsVal += tokVal;
      }else if(!strcmp(pcC[0], "ravg")){
        *rowPcCsVal += tokVal;
        (*rowPcCsCt)++;
      }else if(!strcmp(pcC[0], "rmin") || !strcmp(pcC[0], "rmax")){
        if(*rowPcCsCt == 0){
          *rowPcCsVal = tokVal;
          (*rowPcCsCt)++;
          //If the rmin or rmax command was not initialized yet, we start with
          //the value from the actual cell
        }else if(!strcmp(pcC[0], "rmin") && tokVal < *rowPcCsVal)
          *rowPcCsVal = tokVal;
        else if(!strcmp(pcC[0], "rmax") && tokVal > *rowPcCsVal)
          *rowPcCsVal = tokVal;
      }else if(!strcmp(pcC[0], "rcount"))
        (*rowPcCsVal)++;
    }
    if(row == arg3 + 1){ 
      //If the actual row is the one to write the output to
      if(!strcmp(pcC[0], "ravg"))
        *rowPcCsVal /= (*rowPcCsCt);
      if(*rowPcCsCt == 0) 
        if(!strcmp(pcC[0], "rmin") 
            || !strcmp(pcC[0], "rmax") 
            || !strcmp(pcC[0], "ravg"))
          return 6; 
          //For commands rmin, rmax and ravg - if none of the processed cells
          //was used to compute the output, write 'NaN' instead
      return 5;
      //Otherwise, write the output number into the cell
    }
  }
  return 1;
  //If no changes have been made
}

//If a processing command was entered, this function is called and it's purpose
//is to call other functions and process their outputs
int pcTheTab(char buffer[], char *del, char token[2*MAXSL], char tempTok[2*MAXSL], 
    int row, int col, char pcC[MAXCA][MAXSL], char slC[MAXCA][MAXSL], 
    double *colPcCsVal, double *rowPcCsVal, int *rowPcCsCt, bool *rseqInit){
  int doIEdFnRet = doIEdFn(slC, buffer, del, row);
  if(doIEdFnRet == 0)
    return -5;
  if(doIEdFnRet == 1)
    if(strcmp(pcC[0], "rseq") && pcC[0][0] == 'r')
      if(row == (atoi(pcC[3]) + 1) || -row == (atoi(pcC[3]) + 1))
        doIEdFnRet = 2;
        //- For row processing commands (except for rseq)
        //If the row is M+1 (M as the third arg), we edit anyways. 
  if(doIEdFnRet == 2){
    int colPcCsFnRet = colPcCsFn(colPcCsVal, buffer, del, pcC, col);
    int rowPcCsFnRet = rowPcCsFn(token, pcC, col, row, rowPcCsVal, rowPcCsCt);
    if(colPcCsFnRet < 0 || colPcCsFnRet > 1)
      return colPcCsFnRet; //Pass the return value, if it is not 0
    if(rowPcCsFnRet < 0 || rowPcCsFnRet > 1)
      return rowPcCsFnRet; //Same thing
    if(!strcmp(pcC[0], "cseq"))
      return cseq(col, pcC, colPcCsVal);
    if(!strcmp(pcC[0], "rseq"))
      return rseq(pcC, row, col, rowPcCsVal, rseqInit);
    char *tolptr;
    int pcCA1 = strtol(pcC[1], &tolptr, 10);
    if(tolptr[0])
      return 0;
    if(col == pcCA1){
      //Cset, tolower, toupper, round and int:
      if(!strcmp(pcC[0], "cset"))
        strcpy(token, pcC[2]);
      else if(pcNumsAndChars(pcC, token))
        return 1;
    }
    if(!strcmp(pcC[0], "copy") || !strcmp(pcC[0], "swap") || !strcmp(pcC[0], "move")){
      //Copy, swap and move
      char *tolptr2;
      int pcCA2 = strtol(pcC[2], &tolptr2, 10);
      if(tolptr2[0])
        return 0;
      if(pcCA1 != pcCA2)
        if(col == pcCA1 || col == pcCA2)
          return cpSpMv(pcC, pcCA1, pcCA2, token, tempTok, col, buffer, del); 
    }
  }
  return 1;
}

/*
** 'main' function
*/

int main(int argc, char *argv[]){
  int delPlc = getDel(argc, argv); 
  //Get info about where the deliter characters are in argv
  char *del = " ";
  if(delPlc < 0) 
    return -delPlc; 
    //-1 means one of the arguments is too long. -2 means there are too many arguments
    //GetDel will return -1 or -2, so from main, we return 1 or 2
  if(delPlc > 0)
    del = argv[delPlc]; 
    //If there are arguments specified, del will point to delimiters in argv
  int edCN = 0; //Amount of editing commands
  int pcCN = 0; //...processing commands
  int slCN = 0; //...
  char pcC[MAXCA][MAXSL];
  char slC[MAXCA][MAXSL];
    //Storing the name and arguments of a processing command and selection command
  char buffer[MAXRL]; 
    //Rows will be temporarily saved here (one at a time) 
  char prevBuffer[MAXRL]; 
    //Necessary because we don't know that a line is the last one, until it's
    //loaded it into the buffer
  char token[2*MAXSL]; 
    //Generally, cells will be temporarily saved here (one at a time). 
    //In some functions, double the size is needed to save two tokens
  int row = 0; 
    //Keeping track of which line is the program actually processing 
    //Negative number means it is the last row
  int col = 0; 
    //Keeping track of which column is the program actually processing
  int firstColN = 1; 
    //Keeping track of total columns count of the first line
  int tempRet = getArgs(argc, argv, delPlc, &edCN, &pcCN, &slCN, pcC, slC);
  if(tempRet < 0)
    return -tempRet; //Error
  if(tempRet == 0){
    //If there are no table editing or processing cmds, just print the table
    while(fgets(buffer, MAXRL, stdin)){
      if(!replaceDels(buffer, del, ++row, &firstColN))
        return 9;
      printf("%s", buffer);
    }
    return 0;
  }
  int finish = 0; 
  //Used when fgets returns EOF and we need to process the last line
  double rowPcCsVal = 0;
  int rowPcCsCt = 0;
  bool rseqInit = false;
  fgets(prevBuffer, MAXRL, stdin);
  while(1){
    strcpy(buffer, prevBuffer); 
    //Load the row into the buffer
    if(!strchr(buffer, '\n')){
      fprintf(stderr, "THE ROW NUMBER '%d' OF YOUR TABLE IS TOO LONG! THE PROGRAM WILL NOW EXIT.\n", row);
      return 8;
    }
    if(!fgets(prevBuffer, MAXRL, stdin) && !finish){ 
      //Read one more row. If the line is empty, it means the previous row was
      //the last one. To indicate that this is the last row, we set row to 
      //a negative number (last run of the while cycle)
      row = -row; 
      finish++;
      continue;
    }else if(finish == 2)
      break;
    else if(finish)
      finish++;
    if(row >= 0)
      row++;
    else
      row--;
    if(!replaceDels(buffer, del, row, &firstColN)) 
      return 9;
      //Returns false if there are different amounts of columns in two rows
    if(edCN)
      irowArow(argc, argv, del, row, firstColN, 0); 
      //The '0' means it is not calling arow, but irow instead
    if(edCN && drowDrows(argc, argv, row))
      continue;
    col = 0;
    int l = 0;
    bool dontPtNextDel = false;
    double colPcCsVal = 0; 
    char tempTok[2*MAXSL];
    for(size_t k = 0; k < strlen(buffer); k++){
      bool dontPtNextTok = false;
      if(buffer[k] == del[0] && (k == 0 || buffer[k-1] == del[0])) 
        //If the first char of a line is a delim or previous char was a delim 
        token[l++] = 0; 
        //Write terminal null as the last tokens char and increase l by one
      if(buffer[k] != del[0] && buffer[k] != '\n'){ 
        //If a char isn't a delim and neither a line break
        token[l] = buffer[k]; 
        //Copy a char from the buffer
        token[++l] = 0; 
        //And increase the l by one, then, put a terminal null after the character
      }else{
        col++;
        if(pcCN == 1){ 
          //If a table processing command was entered by the user
          int pcTheTabRet = pcTheTab(buffer, del, token, tempTok, row, col, pcC, slC, &colPcCsVal, &rowPcCsVal, &rowPcCsCt, &rseqInit);
          //Get return code from the table processing function 
          if(pcTheTabRet < 0)
            return -pcTheTabRet; //Error
          if(pcTheTabRet == 2)
            dontPtNextTok = true;
            //For example, if a column is moved, it should not be printed at
            //it's previous position
          if(pcTheTabRet == 2 || pcTheTabRet == 3)
            dontPtNextDel = true;
          if(pcTheTabRet == 4 || pcTheTabRet == 5){
            //If the output value should be written to the table
            double outVar = colPcCsVal;
            double tempVar = colPcCsVal;
            if(pcTheTabRet == 5)
              tempVar = outVar = rowPcCsVal;
            if(tempVar > 0)
              while(tempVar >= 1)
                tempVar--;
            else if(tempVar < 0)
              while(tempVar <= -1)
                tempVar++;
            if(tempVar > -1 && tempVar < 1)
              sprintf(token, "%g", outVar); 
              //Print it as a double, if it has a decimal digit
            else
              sprintf(token, "%d", (int)outVar);
              //Otherwise, print it without the decimal point
          }
          if(pcTheTabRet == 6)
            strcpy(token, "NaN");
            //Print 'NaN' if there were supposed to be numbers to work with,
            //but there were none
        }else if(edCN)
          icolAcol(col, firstColN, argc, argv, del, 0); 
          //0 means it is not executing acol, but icol instead
        if(edCN && dcolDcols(argc, argv, col)){
          dontPtNextTok = true;
          //If a column was deleted, just don't print it
          if(col == 1)
            dontPtNextDel = true;
            //If the first column was deleted, don't print the delimiter either
        }
        if(col && !strchr(token, '\n') && !dontPtNextTok){
          //This is where the columns are printed one by one
          if(col > 1 && !dontPtNextDel)
            printf("%c", del[0]);
          printf("%s", token);
          token[0] = 0;
          dontPtNextDel = false;
        }
        if(edCN)
          icolAcol(col, firstColN, argc, argv, del, 1); 
          //1 indicating that this calling is executing acol
        l = 0;
      }
    }
    printf("\n");
  }
  if(edCN)
    irowArow(argc, argv, del, row, firstColN, 1); 
    //1 indicating that this calling is executing arow
  return 0;
}

