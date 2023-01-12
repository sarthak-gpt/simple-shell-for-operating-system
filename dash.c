/*

Created by Sarthak Gupta for Operating Systems CS 5348 graduate course
Copyright © sarthak-gpt. All rights reserved.

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

void printError(){
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

char* readLine(void){
    size_t n = 100;
    char* line = malloc(sizeof(char) * n);

    if (getline(&line, &n, stdin) == -1) // add functionality for EOF when we get -1
        printError(); //printf("Error in getline\n");

    line[strcspn(line,"\n")] = 0; // replacing last newline character with null as getline takes newline too
    
    return line;
}

char** splitLine(char* line, char* delim){
    char** tokens = malloc(sizeof(char*) * 100);
    char* token;

    int i = 0;
    while ((token = strtok_r(line, delim, &line)))
        tokens[i++] = token;
        
    tokens[i] = NULL;
    return tokens;

}

int interactive(char* line_in_batch, char** paths) {
    
    bool continue_loop = true;
    char* ampersand_delim = "&"; 
    char* redirect_delim = ">"; 
    char* space_delim = " "; 
    char* input_line;
    char** ampersand_arr;
    
    pid_t child_pid, wpid;
    int status = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    do {
        
        if (line_in_batch[0] == '\0') { //if line_in_batch is NULL means it is interactive, else it means it is batch
            printf("dash> ");
            input_line = readLine();
        }
        else {
            input_line = strdup(line_in_batch); //printf("Batch line = |%s|\n", line_in_batch);
            continue_loop = false;
        }
        
        
        if (strlen(input_line) == 0) // Empty Line
            return 0;
        
        int ampersand_count = 0;
       
        for (k = 0; input_line[k] != '\0'; k++)
            if (input_line[k] == '&')
                ampersand_count++;

        ampersand_arr = splitLine(input_line, ampersand_delim);

        if (ampersand_count > 0 && ampersand_arr[0] == NULL){
            printError(); // multiple ampersands but not a single command given;
            continue;
        }

        for (i = 0; ampersand_arr[i] != NULL && i < 30; i++) {
            
            bool isRedirect = false;
            int count_redirect = 0;
            
            for (j = 0; ampersand_arr[i][j] != '\0'; j++)
                if (ampersand_arr[i][j] == '>')
                    count_redirect++;

            if (count_redirect > 1) {//printf("Multiple redirect operators in single command\n");
                printError(); 
                continue;
            }
            char** redirect_arr = splitLine(ampersand_arr[i], redirect_delim);
            
            if (count_redirect == 1 ){ // Single Redirect present but invalid inputs to left or right
                if (redirect_arr[0] == NULL || redirect_arr[1] == NULL || splitLine(redirect_arr[1], space_delim)[1] != NULL) {
                    printError(); //printf("Error: There are multiple output files for redirection\n");
                    continue;
                }

                isRedirect = true;
            }

            char** args = splitLine(redirect_arr[0], space_delim);

            if (args[0] == NULL) { // No command given in between &
                continue; //printError(); // printf("Error: Null command\n");
            }
                

            else if (strcmp(args[0], "exit") == 0) { // checking for inbuild command exit
                if (args[1] != NULL) 
                    printError(); //printf("exit command can't have any arguments\n");
                else
                    exit(0);
            }

            else if (strcmp(args[0], "cd") == 0) { // checking for inbuilt command cd
                if (args[1] == NULL || args[2] != NULL)
                    printError(); //printf("cd must have only 1 argument\n");
                else if (chdir(args[1]) == -1)
                    printError(); //printf("chdir command did not work\n");
            }

            else if (strcmp(args[0], "path") == 0) { // checking for inbuilt command path
                for (j = 0; paths[j] != NULL; j++)
                    paths[j] = NULL; // setting previos path to null
                
                for (j = 1; args[j] != NULL; j++) 
                    paths[j-1] = strdup(args[j]); //Updating path values to current
                        
            }

            else { // checking for external command
                bool found_executable = false;
                
                for (j = 0; paths[j] != NULL; j++) {

                    char* executable = strdup(paths[j]);
                    if (executable[strlen(executable) - 1] != '/')
                        strcat(executable, "/");
                    strcat(executable, strdup(args[0]));
                    //printf("Entering access of |%s|\n", executable);
                    if (access(executable, X_OK) == 0) {
                        //printf("Access of %s was successful\n", executable);
                        found_executable = true;
                        args[0] = strdup(executable); // CHANGING
                        // Need to execute_process(args);

                        if ((child_pid = fork()) == 0) {
                            //child code

                            if (isRedirect == true) {
                                char* output = splitLine(strdup(redirect_arr[1]), space_delim)[0];
                                
                                int file = creat(output, 0777);
                                dup2(file, STDOUT_FILENO);
                                dup2(file, STDERR_FILENO);
                                execv(args[0], args);
                                close(file);
                                free(output);
                            }

                            else { // No redirect
                                //printf("Entering execv for %s\n", args[0]);

                                if (execv(args[0], args) == -1) {
                                    printError(); //printf("Execv failed\n");
                                }
                                    
                                
                            }
                            exit(0);
                        }

                        break;

                    }
                    free(executable);
                }

                if (found_executable == false) {
                    printError(); //printf("Executable was not found in any paths\n");
                }
                    

            }
            free(args);
            free(redirect_arr);
        }

        while ((wpid = wait(&status)) > 0);

    }while (continue_loop == true);

    //free(paths); 
    free(ampersand_arr); free(input_line);

    return 0;
}

int main(int argc, char** argv){
    char** paths = malloc(sizeof(char*));
    paths[0] = malloc(sizeof(char));
    strcat(paths[0],"/bin");

    if (argc == 1)
        interactive(malloc(sizeof(char)), paths);
    else if (argc == 2) {
        FILE* fp;
        char* line_in_batch = malloc(sizeof(char));
        size_t len = 100;
        ssize_t read;

        fp = fopen(argv[1], "r");
        
        if (fp == NULL)
            exit(1);
        
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        
        if (size == 0) { // checking for empty file
            printError();
            exit(1);
        }

        fp = fopen(argv[1], "r");
       
        while ((read = getline(&line_in_batch, &len, fp)) != -1) {
            if (strlen(line_in_batch) == 1) // Empty line -> just a \n character received
                continue;
            line_in_batch[strcspn(line_in_batch,"\n")] = 0; // removing newline char from line
            interactive(line_in_batch, paths);
        }

        fclose(fp);

        if (line_in_batch != NULL)
            free(line_in_batch);
    }
        
    else  {
        printError(); //printf("Invalid Args \n");
        exit(1);
    }
    
    free(paths);

    return 0;
}

