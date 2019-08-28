#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>


void status(int exitStatus, int killedBySig)
{
    if (killedBySig == 0)
    {
        // last command not killed by signal
        printf("exit value %d\n", exitStatus);
        fflush(stdout);
    }
    else
    {
        // last command was killed by a signal
        printf("terminated by signal %d\n", exitStatus);
        fflush(stdout);
    }
}

void cd(char *args[])
{
    if (args[1] == NULL)
    {
        // no second arg provided, cd to home dir
        if (chdir(getenv("HOME")) != 0)
        {
            perror("ERROR");
            fflush(stderr);
        }
    }
    else
    {
        // second arg privided, attempt to cd to given dir
        if (chdir(args[1]) != 0)
        {
            perror("ERROR");
            fflush(stderr);
        }
    }
    return;
}
