//IZP_Projekt_1_Echo.exe -d "|"


#include <stdio.h>
#include <stdlib.h>

#define maxChars 1000

char getTableSize(char separator, int colCountTotal, int rowCountTotal, FILE *file, char row[maxChars], char *filename){
	int i, j, k, l;
	i = j = k = l = 0;
	
    int colCount = 0;
    
 	char table[maxChars][maxChars];
	
 
    file = fopen(filename, "r");
    if (file == NULL){
        printf("The file does not exist");
        return 1;
    }
    
    while (fgets(row, maxChars, file) != NULL){
		if(row[0] != '\0'){
			colCount = 1;	
			while(row[j] != '\0'){
				if (row[j] == separator){
					colCount++;
				}
				j++;
			}	
		}
		if (colCount > colCountTotal){
			colCountTotal = colCount;
			colCount = 0;
		}
        rowCountTotal++;
	}
	fclose(file);
}

int main(int argc, char *argv[]) {	
	
    int *colCountTotal = 0;
    int *rowCountTotal = 0;
    char separator;
    char command[100];
	int argi = 1;
	
		//printf("%c\n", argv[argi]);
	if(argv[argi] == "-d"){
		argi++;
    	separator = *argv[argi];
    	argi++;
	}
	else{
    	separator = ' ';		
	}
    strcpy(command, argv[argi]);
	//command = *argv[argi];
    
	printf("Separator: %c\n", separator);
	printf("Command: %s\n", command);
    
    //printf("%10s", argv[argc-1]);
    
	int i;
	for(i = 0; i < 10; i++){
		printf("%10s", argv[i]);		
	}
	
	//char separator = '|';
	
	
	/*
	FILE *file;
    char row[maxChars];
    char *filename = "c:\\test1.txt";
	*/
    return 0;
}
