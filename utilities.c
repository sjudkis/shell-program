#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>



int myStrLen(char *input)
{
    int count = 0; 
    int i = 0;

    while (input[i] != '\0')
    {
        count++;
        i++;
    }
    return count;
}


void insertPID(char* buffer, char* input)
{
    char target[3] = "$$";
    // get process id
    int pidNum= getpid();
    // convert pid to string
    char* pidStr = malloc(sizeof(char) * 15);
    memset(pidStr, '\0', 15);
    sprintf(pidStr, "%d", pidNum);

    // length of pid
    int pidLen = myStrLen(pidStr);

    char *foundPtr;

    foundPtr = strstr(input, target);
    
    // while $$ is still found in input string
    int i = 0; // index for input string
    int b = 0; // index for buffer string
    while (foundPtr != NULL)
    {

        // copy chars from input to buffer until start of target is found
        while (foundPtr != &input[i])
        {
            buffer[b] = input[i];
            b++;
            i++;
        }
        // target found, copy pid onto end of buffer
        strcat(buffer, pidStr);
        b += pidLen; // increment buffer ptr by length of pid
        i += 2; // increment input ptr past the current $$

        foundPtr = strstr(&input[i], target);


    }
    // no more $$ found, copy rest of input to buffer
    strcat(buffer, &input[i]);

    free(pidStr);
}

void freeArray(char* array[], int num)
{
    int i;
    for (i=0; i < num; i++)
    {
        if (array[i] != NULL)
        {
            free(array[i]);
            array[i] = NULL;
        }
    }
}
