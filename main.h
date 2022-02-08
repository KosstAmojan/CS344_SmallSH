// Name: Matthew Norwood
// OSU Email: norwooma@oregonstate.edu
// Course: CS344 - Operating Systems
// Assignment: 3
// Due Date: 2/8/22
// Description: This is the header file to be used by main.c

#ifndef ASSIGNMENT3_MAIN_H
#define ASSIGNMENT3_MAIN_H
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "main.h"

// Macro definitions here //
#define MAX_LINE 2047
#define MAX_PATH 4095
#define ARG_MAX 256
#define PROC_MAX 200

// Added strtok_r function prototype because of warnings.
extern char *strtok_r(char *, const char *, char **);

// File scope variables for signal handlers
volatile sig_atomic_t interrupt_sig = 0;
volatile sig_atomic_t tempstop_sig = 0;


// Struct definitions here //
struct command_data {
    const char *command;
    char *argumentVector[ARG_MAX];
    char *input_file;
    char *output_file;
    int run_in_background;
};

struct process_status {
    int lastExitStatus;
    int lastTermSig;
    int childCount;
    pid_t unfinishedProcessArray[PROC_MAX];
};

void handle_SIGCONT(int signo){
    int savederrno = errno;

    errno = savederrno;
}

void handle_TEMPSTOP(int signo){
    int savederrno = errno;

    errno = savederrno;
}

ssize_t pidDigitCount(pid_t process_id){
    ssize_t pidDigitCnt = 0;

    // count the number of digits in the shell_process_id
    while (process_id > 1) {
        pidDigitCnt++;
        process_id /= 10;
    }

    return pidDigitCnt;
}

char* pidConvert(pid_t process_id){
    char *pidString = NULL;

    ssize_t pidDigitCnt = 0;
    pidDigitCnt = pidDigitCount(process_id);

    // convert pid to string
    pidString = malloc(sizeof(size_t) * pidDigitCnt + 1);
    if (sprintf(pidString, "%d",process_id) < 0) {
//        perror("sprintf pidString error: ");
    }
    fflush(stdout);

    return pidString;

}

int changeDirectory(struct command_data *curr_command) {

//    char pwd[MAX_PATH] = {};
//    char cwd[MAX_PATH] = {};
    char exp_tilde[MAX_PATH] = {};

//    printf("%s\n",getcwd(pwd,MAX_PATH));
//    fflush(stdout);


    if (curr_command->argumentVector[1] == NULL) {
        // change working directory to directory stored in Home variable.
        if (getenv("HOME") != NULL) {
            if (chdir(getenv("HOME")) != 0){
//                perror("Error changing wd to HOME.");
            };
        }
    }

    else if (curr_command->argumentVector[1][0] == '~') {
        if (getenv("HOME") != NULL) {
            strcat(exp_tilde, getenv("HOME"));
            strcat(exp_tilde, curr_command->argumentVector[1]+1);
            if (chdir(exp_tilde) != 0){
//                perror("Invalid path.");
            };
        }
    }

    else if (chdir((curr_command->argumentVector[1])) != 0) {
//        perror("Invalid path.");
        return -1;
    }
//    printf("%s\n",getcwd(cwd,MAX_PATH));
    printf("\n");
    fflush(stdout);
    return 0;

}

void killAll(void){
    // 0 will kill all the processes in the parent's process group. SIGTERM allows the parent process
    // to perform clean up.
    kill(0,SIGTERM);

}

void status(struct process_status *last_p_status){
    /// gets the exit status or term signal of the last foreground process run by the shell.
    if (last_p_status->childCount == 0) {
        printf("exit value: %d", last_p_status->lastExitStatus);
        fflush(stdout);
    }

    // add code to handle signals once we get there.
    printf("\n");
    fflush(stdout);
}

int input_output_redirect(struct command_data *curr_command, const char *input_or_output,
                                                    struct process_status *last_p_status) {

    char *filestream;
    if (strcmp(input_or_output, "input") == 0) {
        filestream = curr_command->input_file;
    }
    else {
        filestream = curr_command->output_file;
    }

    char *filepath;
    if (filestream == NULL && curr_command->run_in_background == 1) {
        filepath = "/dev/null";
    } else {
        filepath = filestream;
    }

    if (filepath != NULL) {
        if (strcmp(input_or_output, "input") == 0) {
            int sourceFD = open(filepath, O_RDONLY);
            if (sourceFD == -1) {
                printf("cannot open %s for input\n", filepath);
                fflush(stdout);
                return 1;
            }

            // Redirect stdin from source file
            int result = dup2(sourceFD, 0);
            if (result == -1) {
                //perror("source dup2()");
                return 1;
            }
        } else {
            int targetFD = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (targetFD == -1) {
                printf("cannot open %s for output\n", filepath);
                fflush(stdout);
                return 1;
            }

            // Redirect stdout to target file
            int result = dup2(targetFD, 1);
            if (result == -1) {
                //perror("target dup2");
                return 1;
            }
        }
    }
    return 0;
}

int processUserInput(struct command_data *curr_command, struct process_status *last_p_status) {

    pid_t parentProc = getpid();

    if (strcmp(curr_command->command, "exit") == 0) {
        killAll();
        return -1;
    } else if (strcmp(curr_command->command, "cd") == 0) {
        if (changeDirectory(curr_command) == 0) {
            return 0;
        } else {
            printf("%s: no such file or directory.\n", curr_command->argumentVector[0]);
            printf("Thread %d: updating last_p_status at line no %d\n", getpid(), __LINE__);
            last_p_status->lastExitStatus = EXIT_FAILURE;
            fflush(stdout);
            return 0;
        }
    } else if (strcmp(curr_command->command, "status") == 0) {
        status(last_p_status);
        return 0;
    } else {

        int childStatus;
        pid_t childPid = fork();

        if (childPid == -1) {
            exit(1);
        } else if (childPid == 0) {
            // make child processes not executed by exec able to be signaled by SIGTERM
            signal(SIGTERM, SIG_DFL);

            // if we're in the child process and it's not a background process don't ignore SIGINT
            if (curr_command->run_in_background == 0) {
                // reset to default signal handler.
                signal(SIGINT, SIG_DFL);
                if (interrupt_sig == 1) {
                    exit(EXIT_FAILURE);
                }
            }

            // print message as child process begins.
            if (curr_command->run_in_background == 1) {
                printf("background pid is %d\n", getpid());
                printf("\n");
                fflush(stdout);
                kill(parentProc, SIGCONT);
            }

            if (input_output_redirect(curr_command, "input", last_p_status) == 1) {
                exit(EXIT_FAILURE);
            }
            if (input_output_redirect(curr_command, "output", last_p_status) == 1) {
                exit(EXIT_FAILURE);
            }

            if (execvp(curr_command->command, curr_command->argumentVector) == -1) {
                printf("%s: no such file or directory.\n", curr_command->argumentVector[0]);
                printf("\n");
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
            printf("\n");
            exit(0);
        } else {// foreground so wait for the child process
            if (curr_command->run_in_background == 0) {
                childPid = waitpid(childPid, &childStatus, 0);
                if (WIFEXITED(childStatus)) {
                    // normal exit
                    if (curr_command->input_file != NULL || curr_command->output_file != NULL) {
                        printf("\n");
                    }
                    if (WEXITSTATUS(childStatus)) {
                        last_p_status->lastExitStatus = EXIT_FAILURE;
                    }
                } else {
                    // abnormal exit
                    if (curr_command->input_file != NULL || curr_command->output_file != NULL) {
                        printf("\n");
                    }
                    printf("terminated by signal %d\n", SIGINT);
                    fflush(stdout);
                }
            } // don't wait for the child process
            else {
                // increment process count.
                last_p_status->childCount++;

                // add process id to the list of unfinished processes.
                last_p_status->unfinishedProcessArray[last_p_status->childCount] = childPid;

                // check the active background processes prior to returning control to main
                // WNOHANG specified. If the child hasn't terminated, waitpid will immediately return with value 0
                if ((childPid = waitpid(-1, &childStatus, WNOHANG)) == -1) {
                    // set the value at that index in the array to zero.
                    for (int k = 0; k < PROC_MAX; k++) {
                        if (last_p_status->unfinishedProcessArray[k] == 0) {
                            continue;
                        }
                        if (last_p_status->unfinishedProcessArray[k] == childPid) {
                            last_p_status->unfinishedProcessArray[k] = 0;
                            last_p_status->childCount--;
                        }
                    }
                    if (WIFEXITED(childStatus)) {
                        if (WEXITSTATUS(childStatus)) {
                            printf("background pid %d is done: exit value %d\n", childPid, WEXITSTATUS(childStatus));
                            last_p_status->lastExitStatus = EXIT_FAILURE;
                            fflush(stdout);
                        } else {
                            printf("background pid %d is done: terminated by signal %d\n", childPid, WTERMSIG(childStatus));
                            fflush(stdout);
                        }
                    }
                    pause();
                }
            }
        }
    }
    return 0;
}

void pidExpand(char* inputString, size_t pidDigitCnt, const char *pidString){

    char *inputStringCpy = malloc(sizeof(*inputString));
    int n = 0, i = 0, j = 0, expand_done = 0;

    // copy contents of old string;
    strcpy(inputStringCpy, inputString);

    // copy character by character from the old string until we get to the symbols $$, then copy from pidString
    while (inputStringCpy[j] != '\0'){
        if (inputStringCpy[j] == '$' && expand_done == 0){
            if (inputStringCpy[n + 1] == '$') {
                for (i = 0; i < pidDigitCnt; i++) {
                    inputString[n] = pidString[i];
                    n++;
                }
                j += 2;
                expand_done++;
                continue;
            }
        }
        inputString[n] = inputStringCpy[n];
        n++;
        j++;
    }

}

void parseCommand2(char *usr_input, struct command_data *command_data) {

    char *saveptr;
    char *usr_input_cpy = calloc(strlen(usr_input) + 1,sizeof(char));
    int arg_fin = 0, arg_ind = 0;

    strcpy(usr_input_cpy, usr_input);

    char *token = strtok_r(usr_input_cpy, " \0", &saveptr);

    // add command to struct
    command_data->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy((char *)command_data->command, token);

    // copy command into first element of argument vector
    command_data->argumentVector[0] = calloc(strlen(command_data->command) + 1,sizeof(char));
    strcpy(command_data->argumentVector[0], command_data->command);
    arg_ind++;

    while (saveptr != NULL) {
        // once we've gotten the command, we can have many variations on the arguments/redirects/background indicator,
        // so just tokenize the string on _ and check the value that was at the spliced index. 
        
        token = strtok_r(NULL, " \0", &saveptr);

        if (token == NULL) {
            break;
        }

            // input file
        if (strcmp((const char *)token, "<") == 0){
            arg_fin++;

            if (saveptr != NULL) {
                token = strtok_r(NULL, " ", &saveptr);
            }

            command_data->input_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command_data->input_file, token);
        }

            // output file
        else if (strcmp((const char *)token, ">") == 0) {
            arg_fin++;

            if (saveptr != NULL) {
                token = strtok_r(NULL, " ", &saveptr);
            }

            command_data->output_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command_data->output_file, token);
        }

            // background process
        else if (strcmp((const char *)token, "&") == 0)
        {
            arg_fin++;
            command_data->run_in_background = 1;

        }

            // argument string
        else if (arg_fin < 1 && arg_ind < ARG_MAX) {

            command_data->argumentVector[arg_ind] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command_data->argumentVector[arg_ind], token);

            arg_ind++;
        }

        if (token == NULL) {
            break;
        }
    }
}

void shellUserInput(void) {
    // This function, called by main.c, handles basic user input/option selection. It then calls the appropriate
    // helper functions as necessary to complete file selection and processing for the user.
    // struct that tracks most recent process status/term signal.
    struct process_status *last_p_status = malloc(sizeof(struct process_status));
    last_p_status->lastExitStatus = 0;
    last_p_status->lastTermSig = 0;
    last_p_status->childCount = 0;
    for (int i = 0; i < 200; i++){
        last_p_status->unfinishedProcessArray[i] = 0;
    }

    // Borrowing signal struct code from lecture materials.
    struct sigaction ignore_action = {{0}};
    struct sigaction continue_action = {{0}};

    // , tempstop_action = {0};
    sigset_t sigset;

    // The ignore_action struct has SIG_IGN as its signal handler
    ignore_action.sa_handler = SIG_IGN;
    continue_action.sa_handler = handle_SIGCONT;

//     Register the ignore_action as the handler for SIGTERM this signal will be ignored.
    sigaction(SIGINT, &ignore_action, NULL);
    sigaction(SIGTERM, &ignore_action, NULL);
    sigaction(SIGCONT, &continue_action, NULL);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGCONT);
    sigaddset(&sigset, SIGTERM);

    sigprocmask(SIGCONT, &sigset, NULL);

    while (1) {

        char **usr_input = calloc(MAX_LINE,  sizeof(char));
        char *usr_input_str = calloc(MAX_LINE,  sizeof(char));
        struct command_data *curr_command = malloc(sizeof(struct command_data));

        // Initialize values to NULL
        curr_command->command = NULL;
        for (int i=0; i<ARG_MAX; i++) {
            curr_command->argumentVector[i] = NULL;
        }
        curr_command->input_file = NULL;
        curr_command->output_file = NULL;
        curr_command->run_in_background = 0;
        char *pidString = NULL;
        int n = 0, childStatus;
        int retval;
        size_t max_line1 = MAX_LINE, *max_line = &max_line1, bytes_read = 0, alloc_mem = sizeof(char) * MAX_LINE,
                            pidDigitCnt = 0;
        pid_t shell_process_id = getpid(), childPid = 0;

        // check for any completed processes, set the value at that index in the array to zero.
        for (int k = 0; k < PROC_MAX; k++) {
            if (last_p_status->unfinishedProcessArray[k] == 0) {
                continue;
            }
            if (last_p_status->unfinishedProcessArray[k] != 0) {
                childPid = last_p_status->unfinishedProcessArray[k];
                if (childPid != 0 && childPid == waitpid(childPid,&childStatus, WNOHANG)) {
                    last_p_status->unfinishedProcessArray[k] = 0;
                    last_p_status->childCount--;
                    if (WIFEXITED(childStatus)) {
                        printf("background pid %d is done: exit value %d\n", childPid, WEXITSTATUS(childStatus));
                        fflush(stdout);
                    } else {
                        printf("background pid %d is done: terminated by signal %d\n", childPid, WTERMSIG(childStatus));
                        fflush(stdout);
                        last_p_status->lastExitStatus = EXIT_FAILURE;
                    }
                }
            }
        }

        // Clear stdin before and after each read from it.

        while (1) {
            printf(":");
            fflush(stdout);

            if ((bytes_read = getline(usr_input, max_line, stdin)) <= 0) {
//            perror("Error reading from getline");
                fflush(stdout);
                continue;
            }

            if (usr_input[0][0] == '#' || usr_input[0][0] == '\n') {
                fflush(stdout);
                if (usr_input[0][0] == '#') {
                    printf("\n");
                }
                continue;
            }
            fflush(stdout);
            break;
        }

        strcpy(usr_input_str, usr_input[0]);
        if (usr_input_str[strlen(usr_input_str)-1] == '\n'){
            usr_input_str[strlen(usr_input_str)-1] = '\0';
        }
        // Strip leading spaces
        while (usr_input_str[0] == ' '){
            usr_input_str++;
            bytes_read--;
        }

       while (usr_input_str[n] != '\0'){
           if (usr_input_str[n] == '$') {
               if (usr_input_str[n+1] == '$'){
                   pidDigitCnt = pidDigitCount(shell_process_id);
                   pidString = pidConvert(shell_process_id);

                   // determine if appending expanded pid to usr_input_str will result in an overflow, if it does, realloc inputString.
                   if ((strlen(pidString) + strlen(usr_input_str) - 1) > alloc_mem) {
                       char *tempString = realloc(usr_input_str, (strlen(pidString)
                                                    + strlen(usr_input_str) - 1));
                       usr_input_str = tempString;
                    }
                   pidExpand(usr_input_str, pidDigitCnt, pidString);
               }
           }
           n++;
       }

       // parse input into command struct.
       if (strlen(usr_input_str) > 0){
         parseCommand2(usr_input_str, curr_command);
       }

        retval = processUserInput(curr_command, last_p_status);

        if (curr_command->command != NULL){
            free((void *)curr_command->command);
        }

        if (curr_command->argumentVector[0] != NULL){
            for (int j = 0; j < ARG_MAX && curr_command->argumentVector[j] != NULL; j++) {
                free(curr_command->argumentVector[j]);
            }
        }
        if (curr_command->input_file != NULL) {
            free(curr_command->input_file);
        }
        if (curr_command->output_file != NULL) {
            free(curr_command->output_file);
        }
        free(curr_command);
        if (pidString != NULL) {
            free(pidString);
        }

        free(*usr_input);
        free(usr_input_str);

        if (retval == -1){
            free(last_p_status);
            return;
        }
    }
}


#endif //ASSIGNMENT3_MAIN_H
