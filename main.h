// Name: Matthew Norwood
// OSU Email: norwooma@oregonstate.edu
// Course: CS344 - Operating Systems
// Assignment: 3
// Due Date: 2/8/22
// Description: This is the header file to be used by main.c


#ifndef ASSIGNMENT3_MAIN_H
#define ASSIGNMENT3_MAIN_H
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

// Macro definitions here //
#define MAX_LINE 2047
#define MAX_PATH 4095
#define ARG_MAX 256

// Struct definitions here //
struct command_data {
    char *command;
    char *argumentList;
    char *argumentVector[ARG_MAX];
    char *input_file;
    char *output_file;
    int run_in_background;
};

struct process_status {
    int lastExitStatus;
    int lastTermSig;
    int processCount;
};

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
        perror("sprintf pidString error: ");
    }

    return pidString;

}

int changeDirectory(struct command_data *curr_command) {

    char pwd[MAX_PATH] = {};
    char cwd[MAX_PATH] = {};
    char exp_tilde[MAX_PATH] = {};

    printf("%s\n",getcwd(pwd,MAX_PATH));


    if (curr_command->argumentList == NULL) {
        // change working directory to directory stored in Home variable.
        if (getenv("HOME") != NULL) {
            if (chdir(getenv("HOME")) != 0){
                perror("Error changing wd to HOME.");
            };
        }
    }

    else if (curr_command->argumentList[0] == '~') {
        if (getenv("HOME") != NULL) {
            strcat(exp_tilde, getenv("HOME"));
            strcat(exp_tilde, curr_command->argumentList+1);
            if (chdir(exp_tilde) != 0){
                perror("Invalid path.");
            };
        }
    }

    else if (chdir(curr_command->argumentList) != 0) {
        perror("Invalid path.");
        return -1;
    }
    printf("%s\n",getcwd(cwd,MAX_PATH));
    return 0;

}

void killAll(void){
    // 0 will kill all the processes in the parent's process group. SIGTERM allows the processes to perform clean up.
    kill(0,SIGTERM);

}

void status(struct process_status *last_p_status){
    /// gets the exit status or term signal of the last foreground process run by the shell.
    if (last_p_status->processCount == 0) {
        printf("exit value: %d", last_p_status->lastExitStatus);
    }

    // add code to handle signals once we get there.
    printf("\n");
}

int processUserInput(struct command_data *curr_command, struct process_status *last_p_status){

    int ret_status = 0;


    if (strcmp(curr_command->command,"exit") == 0){
        killAll();
        return 1;
    }

    else if (strcmp(curr_command->command,"cd") == 0){
        if (changeDirectory(curr_command) == 0) {
            return ret_status;
        }
        else return -1;
    }

    else if (strcmp(curr_command->command,"status") == 0){
        status(last_p_status);
        return 0;
    }

    else {
        int childStatus;
        pid_t childPid = fork();

        if(childPid == -1){
            perror("fork() failed!");
            exit(1);
        } else if(childPid == 0) {
            // this is the branch for the child process
            exit(1);
        } else {
            // this the branch for the parent process
        }
        // non-built in command, need to check path for matching executable.
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

void parseCommand(char *usr_input, struct command_data *command_data){

    char *saveptr;
    char *usr_input_cpy = calloc(strlen(usr_input) + 1,sizeof(char));
    long cpy_ptr_start = (long)usr_input_cpy;
    long delim_index = 0;
//    size_t run_token_len = 0;
    int str_trail = 1;

    strcpy(usr_input_cpy, usr_input);

    char *token = strtok_r(usr_input_cpy, " ", &saveptr);
    command_data->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(command_data->command, token);

    // subtract pointers to track location of delimiter in old string, update this with every call to strtok_r.
    // don't subtract one here because i don't care about the space.

    token = strtok_r(NULL, "<>&\0", &saveptr);

    if (token == NULL){
        free(usr_input_cpy);
        return;
    }

    if (saveptr != NULL) {
        delim_index = (long)saveptr - cpy_ptr_start - 1;
    }

    else {
        delim_index = strlen(command_data->command);
    }

    // anything between
    if(delim_index > strlen(command_data->command) - 1 && token != NULL) {

        // make the trailing space(s) null terminator(s) (if there are trailing space(s)).
        while (token[strlen(token) - str_trail] == ' '){
            token[strlen(token) - 1] = '\0';
            str_trail++;
        }
        str_trail = 1;

        // advance the pointer past the first space
        while (token[0] == ' ') {
            token++;
        }

        command_data->argumentList = calloc(strlen(token) + 1, sizeof(char));
        strcpy(command_data->argumentList, token);

    }

    // input file
    if (usr_input[delim_index] == '<' && usr_input[delim_index + 1] == ' ') {
        token = strtok_r(NULL, "<>&\0", &saveptr);

        if (token == NULL){
            free(usr_input_cpy);
            return;
        }

        if (saveptr != NULL) {
            delim_index = (long)saveptr - cpy_ptr_start - 1;
        }
        else {
            delim_index = (long)strlen(usr_input) - 1;
        }


        // make the trailing space(s) null terminator(s) (if there are a trailing space(s)).
        while (token[strlen(token) - str_trail] == ' '){
            token[strlen(token) - 1] = '\0';
            str_trail++;
        }
        str_trail = 1;

        // advance the pointer past the first space
        while (token[0] == ' ') {
            token++;
        }

        command_data->input_file = calloc(strlen(token) + 1, sizeof(char));
        strcpy(command_data->input_file, token);
    }

    // output file
    if (usr_input[delim_index] == '>' && usr_input[delim_index + 1] == ' ') {

        token = strtok_r(NULL, "<>&\0", &saveptr);

        if (token == NULL){
            free(usr_input_cpy);
            return;
        }

        if (saveptr != NULL) {
            delim_index = (long)saveptr - cpy_ptr_start - 1;
        }
        else {
            delim_index = (long)strlen(usr_input) - 1;
        }


        // make the trailing space(s) null terminator(s) (if there are a trailing space(s)).
        while (token[strlen(token) - str_trail] == ' '){
            token[strlen(token) - 1] = '\0';
            str_trail++;
        }
        str_trail = 1;

        // advance the pointer past the first space
        while (token[0] == ' ') {
            token++;
        }
        command_data->output_file = calloc(strlen(token) + 1, sizeof(char));
        strcpy(command_data->output_file, token);
    }

    // background process

    if (usr_input[delim_index] == '&' && (usr_input[delim_index + 1] == ' ' || usr_input[delim_index + 1]== '\0')) {
        command_data->run_in_background = 1;
    }

    free(usr_input_cpy);
}

void argumentVector(struct command_data *command_data) {

    char *str_command_copy = calloc(strlen(command_data->argumentList),sizeof(char));
    strcpy(str_command_copy, command_data->argumentList);
    char *saveptr;
    char *token;

    for (int i = 0; i < ARG_MAX; i++){
        if (i == 0) {
            token = strtok_r(str_command_copy, " \0", &saveptr);
        }
        else {
            token = strtok_r(NULL, " \0", &saveptr);
        }

        if (token == NULL) {
            break;
        }
        command_data->argumentVector[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(command_data->argumentVector[i], token);
    }
    free(str_command_copy);
}

void shellUserInput(void) {
    // This function, called by main.c, handles basic user input/option selection. It then calls the appropriate
    // helper functions as necessary to complete file selection and processing for the user.

    // struct that tracks most recent process status/term signal.
    struct process_status *last_p_status = malloc(sizeof(struct process_status));
    last_p_status->lastExitStatus = 0;
    last_p_status->lastTermSig = 0;
    last_p_status->processCount = 0;

    // Borrowing signal struct code from lecture materials.
    struct sigaction ignore_action = {0};
    int lastStatus = 0;

    // The ignore_action struct as SIG_IGN as its signal handler
    ignore_action.sa_handler = SIG_IGN;

    // Register the ignore_action as the handler for SIGTERM this signal will be ignored.
    sigaction(SIGTERM, &ignore_action, NULL);

    while (1) {

        char **usr_input = calloc(MAX_LINE,  sizeof(char));
        char *usr_input_str = calloc(MAX_LINE,  sizeof(char));
        struct command_data *curr_command = malloc(sizeof(struct command_data));
        // Initialize values to NULL
        curr_command->command = NULL;
        curr_command->argumentList = NULL;
        for (int i=0; i<ARG_MAX; i++) {
            curr_command->argumentVector[i] = NULL;
        }
        curr_command->input_file = NULL;
        curr_command->output_file = NULL;
        curr_command->run_in_background = 0;
        char *pidString = NULL;
        int n = 0, i = 0, status = 0;
        size_t max_line1 = MAX_LINE, *max_line = &max_line1, bytes_read = 0, alloc_mem = sizeof(char) * MAX_LINE,
                            pidDigitCnt = 0;
        pid_t shell_process_id = getpid();
        printf(":");

        if ((bytes_read = getline(usr_input, max_line, stdin)) < 0) {
            perror("Error reading from getline");
            }

        // Clear stdin after each read from it.
        fflush(stdin);

        if (bytes_read == 1) {
            continue;
        }

//        if ((*usr_input)[bytes_read - 1] == '\n') {
//            (*usr_input)[bytes_read - 1] = '\0';
//        }

        strncpy(usr_input_str, usr_input[0], strlen(*usr_input) - 2);

        // Strip leading spaces
        while (usr_input_str[0] == ' '){
            usr_input_str++;
            bytes_read--;
        }

        // Comment line, ignore
        if (usr_input_str[0] == '#') {
            continue;
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
                   bytes_read = strlen(usr_input_str);
               }
           }
           n++;
       }

       // parse input into command struct.
       if (strlen(usr_input_str) > 0){
           parseCommand(usr_input_str, curr_command);
       }

       // create argument vector.
        if (curr_command->argumentList != NULL){
            argumentVector(curr_command);
        }

       //printf("command: %s\narguments: %s\ninput: %s\noutput: %s\nbackground %d\n",
//              curr_command->command, curr_command->argumentList, curr_command->input_file, curr_command->output_file,
//              curr_command->run_in_background);

       status = processUserInput(curr_command, last_p_status);
       //printf("The string the user entered was: '%s' and it was %d bytes long.\n", usr_input_str, (int)bytes_read);

        if (curr_command->command != NULL){
            free(curr_command->command);
        }
        if (curr_command->argumentList != NULL){
            free(curr_command->argumentList);
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

       i++;
       if (i == 10 || status == 1) {
           free(last_p_status);
           break;
       }

    }
}



#endif //ASSIGNMENT3_MAIN_H
