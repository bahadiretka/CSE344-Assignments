#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define MAX_INPUT 1024
#define MAX_COMMANDS 20

/*
This function takes the name of a log file, an array of commands, an array of PIDs,
and the number of commands as arguments. It opens the log file for writing, and then
loops through the arrays, writing each command and its associated PID to the file. 
Finally, it closes the file.
*/
void log_commands(const char *filename, char *commands[], int pids[], int num_commands) {
    FILE *log = fopen(filename, "w");
    if (!log) {
        perror("fopen");
        return;
    }
    
    for (int i = 0; i < num_commands; i++) {
        fprintf(log, "PID: %d, Command: %s\n", pids[i], commands[i]);
    }
    
    fclose(log);
}
/*
This function generates a timestamp in the format "YYYY-MM-DD_HH-MM-SS",
and returns a string containing the timestamp. 
It uses the time() function from the time.h library to get the 
current time, and the strftime() function to format it.
*/
char *timestamp() {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char *formatted_time = malloc(20);
    strftime(formatted_time, 20, "%Y-%m-%d_%H-%M-%S", timeinfo);
    char *result = strdup(formatted_time);
    free(formatted_time);
    return result;
}
/*
This function takes a command, an input file descriptor, and an output file descriptor 
as arguments. It duplicates the input and output file descriptors if they are not 
the standard input or output file descriptors, closes the original file descriptors, and
then executes the command using the shell ("/bin/sh"). If the execl() function fails, 
it prints an error message and exits with an error code.
*/
void execute_command(char *command, int in_fd, int out_fd) {
    if (in_fd != STDIN_FILENO) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    if (out_fd != STDOUT_FILENO) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    char *path = "/bin/sh";
    execl(path, path, "-c", command, NULL);
    perror("execl");
    exit(1);
}
/*
This function takes a string containing one or more commands separated by "|" characters as input.
It splits the string into an array of commands, and then loops through the commands. 
For each command, it forks a new process and executes the command in that process using
the execute_command() function. If there are multiple commands, it sets up a pipe between 
the processes to pass the output of one command to the input of the next command. It also 
logs the commands and their associated PIDs to a file using the log_commands() function.
*/
void run_commands(char *input) {
    int num_commands = 0;
    char *commands[MAX_COMMANDS];
    int pids[MAX_COMMANDS];

    char *command = strtok(input, "|");
    while (command && num_commands < MAX_COMMANDS) {
        commands[num_commands++] = command;
        command = strtok(NULL, "|");
    }

    int pipe_fds[2];
    int in_fd = STDIN_FILENO;
    int out_fd = STDOUT_FILENO;

    for (int i = 0; i < num_commands; i++) {
        if (i < num_commands - 1) {
            pipe(pipe_fds);
            out_fd = pipe_fds[1];
        } else {
            out_fd = STDOUT_FILENO;
        }

        int pid = fork();
        if (pid == 0) {
            execute_command(commands[i], in_fd, out_fd);
        } else if (pid > 0) {
            pids[i] = pid;
            if (in_fd != STDIN_FILENO) {
                close(in_fd);
            }
            if (out_fd != STDOUT_FILENO) {
                close(out_fd);
            }
            in_fd = pipe_fds[0];
        } else {
            perror("fork");
            exit(1);
        }
    }

    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }

    char *log_filename = timestamp();
    log_commands(log_filename, commands, pids, num_commands);
    free(log_filename);
}
/*
This is the main function of the program. It takes two arguments: argc, the number
of command-line arguments passed to the program, and argv, an array of strings
containing the command-line arguments. If the number of arguments is not equal to 1, 
it prints a usage message and returns an error code. Otherwise, it enters a loop that 
prompts the user for input using the fgets() function, runs the input through the 
run_commands() function, and repeats until the user enters ":q" to quit.
*/
int main(int argc, char *argv[]) {
    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        return 1;
    }

    char input[MAX_INPUT];
    while (1) {
        printf("bek_shell> ");
        fgets(input, MAX_INPUT, stdin);
        input[strcspn(input, "\n")] = '\0'; 
                if (strcmp(input, ":q") == 0) {
            break;
        } else {
            run_commands(input);
        }
    }

    return 0;
}

