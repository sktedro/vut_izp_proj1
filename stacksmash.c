#include <stdio.h>
#include <string.h>

int main(void)
{
    int len = 0;
    char str[10] = {0};

    printf("\n Enter the name \n");

    gets(str); // Used gets() to cause buffer overflow

    printf("\n len = [%d] \n", len);

    len  = strlen(str);
    printf("\n len of string entered is : [%d]\n", len);

    return 0;
}
