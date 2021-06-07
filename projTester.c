/*
** This program tests your IZP project with pretty much random inputs.
** 
** Any ideas about improvements are welcome.
** 
** If you know how to write output of the terminal (segfault or stack smashing
** error, to be more specific), please help us all. This version stops executing 
** commands when one of these two errors is detected. 
** It would be far better if it did not stop, but continue instead, 
** and only write the input and output to a desired file.
** 
** Tedro. No rights reversed.
**
** To use:
  ** change the name of the filename with your program (which is in the same
  ** folder as this one.
  **
  ** change the name of the input table to yours.
  **
  ** be careful with execsPerSec. Too high of a number can mean some trouble
  **
  ** if you want the testing to stop, press CTRL+C (should work both on linux and
  ** windowsf)
  **
  ** you can uncomment the first string in array "args[][]" (and increment int
  ** argsnum by one) if you wish your output tables to be written into a file.
  ** This seems unnecessary tho, because this program is only supposed to
  ** search for errors.
  **
  ** you can, of course, edit arguments you wish to be tried. But don't forget
  ** to change "int argsNum" var, which should represent the number of strings
  ** in args[][].
  **
  ** No idea about how to make an argument be tried in "". All args are tested
  ** as plain text.
  **
  ** Uncomment the second row of cmds[][] if you implemented the optional commands
  ** too.
  **
  ** This is just to test for errors. I might and might not make another program 
  ** to help check if your outputs are right. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define maxArgLen 100

#define maxExecLen 20 //max number of commands by execution - if you wish to try fewer or more args per execution
#define execsPerSec 1000 //how many tests each second (1, 10, 100, 1000, 10 000, 100 000, and maximum 1 000 000)
#define tryZeroArgs 0 //put 1 if you'd also like your program to be tested with no arguments
#define writeToFile 1 
  /*if 1, writes output (only inputs, which caused an error) to an output.txt file.
  ** It will still show return values of your program on the terminal, so you can see what's going on.
  ** Otherwise (when set to 0) writes every input and output into the terminal and 
  ** stops if an error is encountered, so you can read the input, which caused the error.
  */

char filename[] = "./main_Victor"; //needs a space after the name!
char inputTable[] = "<tab1.txt";

char cmds[][11] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols",
                   "csum", "cavg", "cmin", "cmax", "ccount", "cseq", "rseq", "rsum", "ravg", "rmin", "rmax", "rcount", //Optional commands
                   "cset", "tolower", "toupper", "round", "int", "copy", "swap", "move"
                    };
char selCmds[][11] = {"rows", "beginswith", "contains"};
char args[][maxArgLen] = {
                  /*">tab2.txt", */ //tab input and output
                  "-d", "_ ", "abcdefghijklmnoprstuvxyz", "1234567890", "adancgryeuairuoubcxssad,ssdwpqtyw98741409836215hjanjsjkah" //delims 
                  "-", "0", "1", "4", "100", "a", "b", "abc", "ABC", "123abc", "abc123", "_1", " 1", " ", "", "_a", " a"}; //other args 
int cmdsNum = 16;
int selCmdsNum = 3;
int argsNum = 21; //22 if you want tab output or different number if you change amount of strings in args[][] 
char space[2] = " ";

int fn(){
  int randHelper = rand()%(maxExecLen);
  int rd;
  for(int i = 0; i < randHelper; i++){
    rd += rand()%(maxExecLen);
    if(rd%10 == 0 && !tryZeroArgs)
      continue;
  }
  return rd;
}

int main(){
  time_t t;
  srand((unsigned) time(&t));

  while(1){
    int returned = 0;

    char command[maxExecLen*maxArgLen];
    command[0] = '\0';
  
    strncat(command, filename, maxArgLen);
    strncat(command, inputTable, maxArgLen); //comment this line if you dont want the input tab to be specified

    int iterations = fn()%maxExecLen;
    if(iterations%5 == 0 && !tryZeroArgs)
      iterations = maxExecLen;
    for(int i = 0; i < iterations; i++){
        strncat(command, space, 1);
        int num = fn()%10;
      if(num >= 1 && num <= 4)
        strncat(command, cmds[fn()%cmdsNum], maxArgLen);
      else if(num >= 5 && num <= 10) 
          strncat(command, args[fn()%argsNum], maxArgLen);
      else
        strncat(command, selCmds[fn()%selCmdsNum], maxArgLen);
    }
  
    //segfault returns 35584
    //Stack smashing/Aborted (Core dumped) returns 34304
    returned = system(command);
    if(!writeToFile){
      printf("Input: %s\n\n", command);
      printf("\nReturned: %d\n", returned);
      if(returned == 35584 || returned == 34304)
        return 1;
    }else{
      if(returned == 35584 || returned == 34304){
        freopen("./output.txt", "a", stdout);
        usleep(100000);
        printf("Input: %s\n", command);
        printf("Returned: %d\n\n", returned);
        fclose(stdout);
        usleep(100000);
      }
    }
    usleep(1000000/execsPerSec); //delay between executions in microseconds!
  }
  return 0;
}
