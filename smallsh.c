// Sam Judkis
// 3/3/19


#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>



// utility function declarations
int myStrLen(char *input);
void insertPID(char* buffer, char* input);
void freeArray(char* array[], int num);

// built in function declarations
void status(int, int);
void cd(char* args[]);



/*********************************************/
// foreground only mode

// global variable, 1 if in foreground mode, 0 if not
int fgMode = 0;

// SIGTSTP signal handler
void catchTSTP(int signo)
{
    // not currently in fg mode
    if (!fgMode)
    {
        // enter fg mode
        fgMode = 1;
        char* msg = "\nEntering foreground-only mode (& is now ignored)\n"; //50
        write(STDOUT_FILENO, msg, 50);
    }
    // currently in fg mode
    else
    {
        // exit fg mode
        fgMode = 0;
        char* msg = "\nExiting foreground-only mode\n"; //30
        write(STDOUT_FILENO, msg, 30);
    }
}


/*******************************************/

int main() {


    int numCharsEntered = -19; // store num chars entered to getline
    size_t bufferSize = 0; // holds size of allocated buffer
    char *lineEntered = NULL; // buffer allocated by getline(). string + \n + \0

    char lineWithPID[10000]; // buffer to hold input string with $$ replaced by pid
    
    // array of char* to hold args passed to shell, plus one for NULL
    char *args[513];


    /*******************************/
    // signal handling
    
    // shell ignores SIGINT signals
    struct sigaction SIGINT_ignore = {{0}};
    SIGINT_ignore.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_ignore.sa_mask);
    SIGINT_ignore.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_ignore, NULL);
    
    // SIGTSTP toggles foreground only mode
    struct sigaction SIGTSTP_handler = {{0}};
    SIGTSTP_handler.sa_handler = catchTSTP;
    sigfillset(&SIGTSTP_handler.sa_mask);
    SIGTSTP_handler.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_handler, NULL);
    /*******************************/




    /*****************************/
    // variables for status function
    // if killedBySig == 1, exitStatus contains signal number that killed the process
    int exitStatus = 0; // hold exit status of last command
    int killedBySig = 0; // boolean, says if last command was terminated by signal


    /************************/
    // array of background PIDs
    int bgArraySize = 256;
    int lastPIDindex = 0;
    pid_t bgPIDs[bgArraySize];


    int i;
    // initialize bg pid array with junk values
    for (i = 0; i < bgArraySize; i++)
        bgPIDs[i] = -19;
 


    // continous loop to get input from user
    while (1)
    {
        // get input from user
        while(1)
        {
            // check for termination of background children
            int j;
            int updateLastIndex = 0;
            int bgExitStatus = -19;
            for (j = 0; j < lastPIDindex + 1; j++)
            {
                // index stores an unharvested PID
                if (bgPIDs[j] != -19)
                {
                    // check if terminated
                    pid_t termResult = waitpid(bgPIDs[j], &bgExitStatus, WNOHANG);
                    
                    // returned nonzero, has terminated
                    if (termResult != 0)
                    {
                        if (WIFEXITED(bgExitStatus))
                        {
                            killedBySig = 0;
                            exitStatus = WEXITSTATUS(bgExitStatus);

                            //printf("%d\n", fgMode);
                            fflush(stdout);
                        }
                        // check if terminated by signal
                        else if (WIFSIGNALED(bgExitStatus))
                        {
                            killedBySig = 1;
                            exitStatus = WTERMSIG(bgExitStatus);
                            fflush(stdout);
                        }

                        // print termination message
                        printf("background pid %d is done: ", bgPIDs[j]);
                        fflush(stdout);
                        status(exitStatus, killedBySig);

                        // reset array index to junk
                        bgPIDs[j] = -19;

                    }

                    // process has not terminated
                    else
                    {
                        // save index as current latest unterminated pid
                        updateLastIndex = j;
                    }
                }
            }
            // save index 
            lastPIDindex = updateLastIndex;
            

            // prompt to user
            printf(": ");
            fflush(stdout);
            
            numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
            if (numCharsEntered == -1)
            {
                clearerr(stdin);
            }
            else
            {
                break; // exit loop
            }
        }
        
        // test for comment
        if (lineEntered[0] == '#')
        {
            free(lineEntered);
            lineEntered = NULL;
            continue;
        }
        // test for blank line
        else if (numCharsEntered == 1)
        {
            free(lineEntered);
            lineEntered = NULL;
            continue;
        }
        

        // remove newline
        lineEntered[strcspn(lineEntered, "\n")] = '\0';
        
        // memset buffer to hold line containing PIDs 
        memset(lineWithPID, '\0', sizeof(char) * 10000);

        // function to replace any $$ with pid
        insertPID(lineWithPID, lineEntered);
      

        // tokenize full line into individual args
        char delim[2] = " "; // char than demliminates args

        // get first arg
        char *token = strtok(lineWithPID, delim);
        
        // check if only spaces entered
        if (token == NULL)
        {
            free(lineEntered);
            lineEntered = NULL;
            continue;
        }

        int numArgs = 0;

        // loop until all args have been found
        while (token != NULL)
        {

            // allocate memory for arg in args array
            args[numArgs] = malloc(sizeof(char) * myStrLen(token) + 1);
            // copy token to args array
            strcpy(args[numArgs], token);
            // get next arg
            token = strtok(NULL, delim);
            // increment counter
            numArgs++;

        }
        // add NULL ptr after args for call to exec
        args[numArgs] = NULL;

        /************************************/
        // user entered status command
        if (strcmp(args[0], "status") == 0)
        {
            status(exitStatus, killedBySig);
        }



        /****************************/
        // user entered cd command
        else if (strcmp(args[0], "cd") == 0)
        {
            // call built in cd command
            cd(args);


        }

        /************************************/
        // user entered exit command
        else if (strcmp(args[0], "exit") == 0)
        {  
            int i;
            // loop through bg process array
            for (i = 0; i < lastPIDindex + 1; i++)
            {
                // kill any bg PIDs still running
                if (bgPIDs[i] != -19)
                {

                    kill(bgPIDs[i], SIGKILL);
                    //printf("killed at exit: %d\n", bgPIDs[i]);
                    fflush(stdout);
                }
            }        
            freeArray(args, numArgs);
            free(lineEntered);
            return 0;
        }

        /*************************************/
        /*      Command exectution           */
        /*************************************/
        // user entered non-built-in command. 
        else
        {
            pid_t spawnPID = -19;
            int childExitStatus = -19;

            // bool, 1 if new process should be background, otherwise 0
            int makebg = 0;

            // check if last arg is &
            if (strcmp(args[numArgs - 1], "&") == 0)
            {
                // remove & arg from array
                free(args[numArgs - 1]);
                args[numArgs - 1] = NULL;

                // decrement arg counter
                numArgs--;
                
                if (!fgMode)
                {
                // if not in fg only mode, set to 1
                    makebg = 1;
                }
            }

            // store arguments for redirection
            char *redirectIn = NULL;
            char *redirectOut = NULL;
            
            // flags, set if input or output is redirected
            int inputProvided = 0;
            int outputProvided = 0;
            
            // store indices to delete
            int inputIndex, outputIndex;

            // parse args for < or >
            for (i = 0; i < numArgs; i++)
            {
                // input redirection found
                if (strcmp(args[i], "<") == 0)
                {
                    // set flag
                    inputProvided = 1;
                    redirectIn = malloc(sizeof(char) * myStrLen(args[i + 1]) + 1);
                    strcpy(redirectIn, args[i + 1]);
                    inputIndex = i;
                }
                else if (strcmp(args[i], ">") == 0)
                {
                    outputProvided = 1;
                    redirectOut = malloc(sizeof(char) * myStrLen(args[i + 1]) + 1);
                    strcpy(redirectOut, args[i + 1]);
                    outputIndex = i;
                }
            }

            // remove redirection arguments from args array
            if (inputProvided)
            {
                free(args[inputIndex]);
                args[inputIndex] = NULL;
                free(args[inputIndex + 1]);
                args[inputIndex + 1] = NULL;
            }
            if (outputProvided)
            {
                free(args[outputIndex]);
                args[outputIndex] = NULL;
                free(args[outputIndex + 1]);
                args[outputIndex + 1] = NULL;
            }


            /********************/
            // fork new process
            /********************/
            spawnPID = fork();

            
            switch(spawnPID)
            {
                // fork failed
                case -1:
                    {
                        perror("Hull Breach!\n");
                        fflush(stderr);
                        exit(1);
                        break;
                    }

                /**********************************/
                // in child process
                case 0:
                    {
                        // set child to ignore SIGTSTP signal
                        struct sigaction SIGTSTP_ignore = {{0}};
                        SIGTSTP_ignore.sa_handler = SIG_IGN;
                        sigfillset(&SIGTSTP_ignore.sa_mask);
                        SIGTSTP_ignore.sa_flags = 0;
                        sigaction(SIGTSTP, &SIGTSTP_ignore, NULL);
                        
                        // stop foreground child from ignoring SIGINT signal
                        if (!makebg)
                        {
                            SIGINT_ignore.sa_handler = SIG_DFL;
                            sigaction(SIGINT, &SIGINT_ignore, NULL);
                        }

                        // redirect input to dev/null for background process if no input provided
                        else
                        {
                            // no redirection given, no file argument to read
                            if (!inputProvided && (args[1] == NULL))
                            {
                                int devNull = open("/dev/null", O_RDONLY);
                                dup2(devNull, 0);

                            }
                            if (!outputProvided)
                            {
                                int devNull = open("/dev/null", O_WRONLY);
                                dup2(devNull, 1);
                                //dup2(devNull, 2);

                            }
                        }

                        // redirect input from given file
                        int badRedirect = 0; // flag
                        char err[32];

                        // open dev/null fd
                        //int devNull = open("/dev/null", O_WRONLY);
                        

                        // currently, invalid input redirection leads to 2 error messages
                        // being printed. One is the message I wrote, the second seems to 
                        // come from the call to open the invalid file. In order to match the 
                        // sample output, I initially figured out a while to stifle the 2nd 
                        // message by redirecting stderr to /dev/null and printing my error 
                        // message with printf instead. This made the output match but seems 
                        // like an improper way to do things. I spoke with Chase Denecke during
                        // office hours and he recommended that I leave it in the original form,
                        // without redirecting stderr, but allowing the extra error message to 
                        // print. 
                        
                        if (inputProvided)
                        {
                            // redirect stderr to avoid default error message from failed open
                            //dup2(devNull, 2);
                            
                        
                            // attempt to open input file 
                            int inPath = open(redirectIn, O_RDONLY);
                           

                            // check for error
                            if (inPath == -1)
                            {
                                badRedirect = 1;
                                memset(err, '\0', sizeof(char) * 32);
                                sprintf(err, "cannot open %s for input\n", redirectIn);
                                perror(err);
                                fflush(stderr);
                            }
                            // input file opened properly
                            else 
                            {
                                // redirect input
                                dup2(inPath, 0);
                            }
                        }
                        // redirect output to given file
                        if (outputProvided)
                        {
                            // attempt to open output file
                            int outPath = open(redirectOut, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            if (outPath == -1)
                            {
                                badRedirect = 1;
                                memset(err, '\0', sizeof(char) * 32);
                                sprintf(err, "cannot open %s for output\n", redirectOut);
                                perror(err);
                                fflush(stderr);
                            }
                            // output file opened properly
                            else 
                            {
                                dup2(outPath, 1);
                            }
                        }
                        if (badRedirect == 1)
                        {
                            exit(1);
                            break;
                        }
                        else
                        {
                            // execute command with execvp
                            execvp(args[0], args);
                            const char* errCmd = args[0];
                            perror(errCmd);
                            fflush(stderr);
                            exit(2);
                        }
                        
                        
                        break;
                    }

                /*************************************/
                // in parent process
                default:
                    {

                        // if child is a foreground process
                        if (!makebg)
                        {
                            // parent should block SIGTSTP until after fg child has terminated
                            sigset_t block_signals;
                            sigemptyset(&block_signals);
                            sigaddset(&block_signals, SIGTSTP);
                            sigprocmask(SIG_BLOCK, &block_signals, NULL);

                            spawnPID = waitpid(spawnPID, &childExitStatus, 0);
                        
                            // check exit status of child process

                            if (WIFEXITED(childExitStatus))
                            {
                                 killedBySig = 0;
                                 exitStatus = WEXITSTATUS(childExitStatus);

                                 //printf("%d\n", fgMode);
                                 fflush(stdout);
                            }
                            else if (WIFSIGNALED(childExitStatus))
                            {
                                 killedBySig = 1;
                                 exitStatus = WTERMSIG(childExitStatus);
                                 status(exitStatus, killedBySig);
                                 fflush(stdout);
                            }

                            // unblock SIGTSTP, allow parent to receive it 
                            sigprocmask(SIG_UNBLOCK, &block_signals, NULL);

                        }

                        // child is a background process
                        else
                        {

                            printf("background pid is %d\n", spawnPID);
                            pid_t tempPID = waitpid(spawnPID, &childExitStatus, WNOHANG);
                            
                            // if not terminated
                            if (tempPID == 0)
                            {
                                // find first open index in bgPIDs array
                                int i = 0;
                                while ((i < lastPIDindex + 1) && (bgPIDs[i] != -19))
                                {
                                    i++;
                                }

                                // store PID in array
                                bgPIDs[i] = spawnPID;
                                if (i > lastPIDindex)
                                {
                                    lastPIDindex = i;
                                }

                            }

                            // child has terminated
                            else
                            {
                                // check if exited normally
                                if (WIFEXITED(childExitStatus))
                                {
                                    killedBySig = 0;
                                    exitStatus = WEXITSTATUS(childExitStatus);

                                    //printf("%d\n", fgMode);
                                    fflush(stdout);
                                }
                                // check if terminated by signal
                                else if (WIFSIGNALED(childExitStatus))
                                {
                                    killedBySig = 1;
                                    exitStatus = WTERMSIG(childExitStatus);
                                    status(exitStatus, killedBySig);
                                    fflush(stdout);
                                }
                            }

                        }


                        // free memory used for redirect files
                        if (redirectIn != NULL)
                        {
                            free(redirectIn);
                            redirectIn = NULL;
                        }
                        if (redirectOut != NULL)
                        {
                            free(redirectOut);
                            redirectOut = NULL;
                        }
                        break;
                    }

            }

        }


        /************************************/
        // free memory allocated by getline
        freeArray(args, numArgs);
        free(lineEntered);
        lineEntered = NULL;
    }



    return 0;
}
