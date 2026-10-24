#include <stdlib.h>
#include <string.h>

#include "string_parser.h"

int count_token(char *buf, const char *delim)
{
    int count = 0;
    char *token;

    if (buf == NULL || delim == NULL) {
        return 0;
    }

    token = strtok(buf, delim);
    while (token != NULL) {
        count++;
        token = strtok(NULL, delim);
    }
    return count;
}

command_line str_filler(char *buf, const char *delim)
{
    command_line command = {NULL, 0};
    char *temporary;
    char *token;
    char *save_pointer;
    int index = 0;

    if (buf == NULL || delim == NULL) {
        return command;
    }

    if (strlen(buf) > 0 && buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
    }

    temporary = malloc(strlen(buf) + 1);
    if (temporary == NULL) {
        return command;
    }
    strcpy(temporary, buf);

    token = strtok_r(temporary, delim, &save_pointer);
    while (token != NULL) {
        command.num_token++;
        token = strtok_r(NULL, delim, &save_pointer);
    }
    free(temporary);

    if (command.num_token == 0) {
        return command;
    }

    command.command_list = malloc((size_t)(command.num_token + 1) *
                                  sizeof(*command.command_list));
    if (command.command_list == NULL) {
        command.num_token = 0;
        return command;
    }

    token = strtok_r(buf, delim, &save_pointer);
    while (token != NULL) {
        command.command_list[index] = malloc(strlen(token) + 1);
        if (command.command_list[index] == NULL) {
            for (int cleanup_index = 0; cleanup_index < index;
                 cleanup_index++) {
                free(command.command_list[cleanup_index]);
            }
            free(command.command_list);
            command.command_list = NULL;
            command.num_token = 0;
            return command;
        }
        strcpy(command.command_list[index], token);
        index++;
        token = strtok_r(NULL, delim, &save_pointer);
    }
    command.command_list[index] = NULL;
    return command;
}

void free_command_line(command_line *command)
{
    if (command == NULL || command->command_list == NULL) {
        return;
    }

    for (int index = 0; index < command->num_token; index++) {
        free(command->command_list[index]);
    }
    free(command->command_list);
    command->command_list = NULL;
    command->num_token = 0;
}
