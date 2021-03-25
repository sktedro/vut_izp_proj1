//IZP_Projekt_1.exe "|"


#include <stdio.h>
#include <stdlib.h>

#define maxChars 1000

char *tableRead(char separator){
	int i, j, k, l;
	i = j = k = l = 0;
	
    int colCount = 0;
    int colCountTotal = 0;
    int rowCountTotal = 0;
    
 	char table[maxChars][maxChars];
	
	FILE *file;
    char row[maxChars];
    char *filename = "c:\\test1.txt";
 
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
		i = 0;
		while(row[i] != '\0'){
        	table[i][rowCountTotal] = row[i];
        	i++;
		}
	}
    fclose(file);
    i = j = 0;
    
    for (j = 0; j <= rowCountTotal; ++j){
    	while(table[i][j] != '\0'){
    		printf("%c", table[i][j]);  
			i++;  		
		}
		i = 0;
	}
	
	//tab[k] are chars in a cell, [l] are columns and [i] are rows
	i = j = k = l = 0;
    printf("\n\n");
    char tab[colCountTotal][colCountTotal][rowCountTotal];
    for(i = 0; i <= rowCountTotal; i++){
    	while(table[j][i] != '\0' && table[j+1][i] != '\0'){
			if(table[j][i] != separator){
    			tab[k][l][i] = table[j][i];
    			printf("%c", tab[k][l][i]);
				k++;
				j++;
			}
			else{
				k = 0;
				l++;
				j++;
				printf(" ");
			}
		}
		j = 0;
		k = 0;
		printf("\n");
	}
//	printf("\n\nColumns count = %d\n", colCountTotal);
//	printf("Rows count = %d\n", rowCountTotal);
	return tab;
}

int main(int argc, char *argv[]) {	
/*
	if (argc == 3){//Tab editing
		//char command[] = *argv[2];
	}
	else if (argc == 4){//Data processing
		
	}
	*/
	
	int *colCountTotal;
	int *rowCountTotal;
	
    //char separator = *argv[1];
	//tableRead(*argv[1]);
	char separator = '|';
	tableRead(separator);
	
    return 0;
}
