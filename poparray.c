#include <stdlib.h>
#include <stdio.h>

int RQ[10] = {1, 4, 2};

int popArrayRQ()
{
    int popped = RQ[0];
    int i = 0;
    while (RQ[i] != 0)
    {
        RQ[i] = RQ[i + 1];
        i++;
    }
    return popped;
}

int firstZeroRQ()
{
    int i = 0;
    while (RQ[i] != 0)
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
