#include <stdlib.h>
#include <stdio.h>

int myarr[10] = {1,4,2};

int popArray()
{
    int popped = myarr[0];
    int i = 0;
    while (myarr[i] != 0)
    {
        myarr[i] = myarr[i + 1];
        i++;
    }
    return popped;
}

int firstZero()
{
    int i = 0;
    while (myarr[i] != 0)
    {
        i++;
    }
    return i;
}


int main(int argc, char *argv[])
{
    printf("%i \n", firstZero());
    exit(EXIT_SUCCESS);
    return 0;
}
