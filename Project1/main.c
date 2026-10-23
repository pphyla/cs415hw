#include "command.h"
#include "string_parser.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void output_text(const char *text)
{
    size_t length = strlen(text);
    size_t written = 0;

    while (written < length) {
        ssize_t result = write(STDOUT_FILENO, text + written, length - written);
        if (result <= 0) {
            return;
        }
        written += (size_t)result;
    }
}

static int expected_arguments(const char *command)
{
    if (strcmp(command, "ls") == 0 || strcmp(command, "pwd") == 0 ||
        strcmp(command, "exit") == 0) {
        return 0;
    }
    if (strcmp(command, "mkdir") == 0 || strcmp(command, "cd") == 0 ||
        strcmp(command, "rm") == 0 || strcmp(command, "cat") == 0) {
        return 1;
    }
    if (strcmp(command, "cp") == 0 || strcmp(command, "mv") == 0) {
        return 2;
    }
    return -1;
}

static int run_command(command_line *parsed)
{
    int required;

    if (parsed->command == NULL) {
        return 1;
    }
    required = expected_arguments(parsed->command);
    if (required == -1) {
        output_text("Error! Unrecognized command: ");
        output_text(parsed->command);
        output_text("\n");
        return -1;
    }
    if (parsed->argument_count != required) {
        output_text("Error! Unsupported parameters for command: ");
        output_text(parsed->command);
        output_text("\n");
        return -1;
    }

    if (strcmp(parsed->command, "ls") == 0) {
        listDir();
    } else if (strcmp(parsed->command, "pwd") == 0) {
        showCurrentDir();
    } else if (strcmp(parsed->command, "mkdir") == 0) {
        makeDir(parsed->arguments[0]);
    } else if (strcmp(parsed->command, "cd") == 0) {
        changeDir(parsed->arguments[0]);
    } else if (strcmp(parsed->command, "cp") == 0) {
        copyFile(parsed->arguments[0], parsed->arguments[1]);
    } else if (strcmp(parsed->command, "mv") == 0) {
        moveFile(parsed->arguments[0], parsed->arguments[1]);
    } else if (strcmp(parsed->command, "rm") == 0) {
        deleteFile(parsed->arguments[0]);
    } else if (strcmp(parsed->command, "cat") == 0) {
        displayFile(parsed->arguments[0]);
    }
    return strcmp(parsed->command, "exit") == 0 ? 0 : 1;
}

static int process_line(char *line)
{
    char *segment;
    char *save_pointer;
    int exit_shell = 0;

    segment = strtok_r(line, ";", &save_pointer);
    while (segment != NULL) {
        command_line *parsed = parse_command_line(segment);
        if (parsed == NULL) {
            return 1;
        }
        if (parsed->command != NULL) {
            int result = run_command(parsed);
            if (result == 0) {
                exit_shell = 1;
            }
            free_command_line(parsed);
            if (result <= 0) {
                break;
            }
        } else {
            free_command_line(parsed);
        }
        segment = strtok_r(NULL, ";", &save_pointer);
    }
    return exit_shell ? 0 : 1;
}

int main(int argc, char **argv)
{
    int input_fd = STDIN_FILENO;
    int output_fd = -1;
    int file_mode = 0;
    char *line = NULL;
    size_t line_capacity = 0;
    ssize_t line_length;

    if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        file_mode = 1;
        input_fd = open(argv[2], O_RDONLY);
        output_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (input_fd == -1 || output_fd == -1) {
            if (input_fd != -1 && input_fd != STDIN_FILENO) {
                close(input_fd);
            }
            if (output_fd != -1) {
                close(output_fd);
            }
            return 1;
        }
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            close(input_fd);
            close(output_fd);
            return 1;
        }
        close(output_fd);
    } else if (argc != 1) {
        return 1;
    }

    if (file_mode) {
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            close(input_fd);
            return 1;
        }
        close(input_fd);
    }

    while (1) {
        if (!file_mode) {
            output_text(">>> ");
        }
        line_length = getline(&line, &line_capacity, stdin);
        if (line_length == -1) {
            break;
        }
        if (!process_line(line)) {
            break;
        }
    }

    free(line);
    return 0;
}
