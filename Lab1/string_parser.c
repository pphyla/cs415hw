/*
 * string_parser.c
 *
 *  Created on: Nov 25, 2020
 *      Author: gguan, Monil
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE

int count_token (char* buf, const char* delim)
{
    if (buf == NULL)
    {
        return 0;
    }
    int count = 0;
    char *copy = malloc(strlen(buf) + 1);
    strcpy(copy, buf);

    char *saveptr;
    char *token = strtok_r(copy, delim, &saveptr);

    while (token != NULL)
    {
        count++;
        token = strtok_r(NULL, delim, &saveptr);
    }

    free(copy);
    return count;
}


command_line str_filler (char* buf, const char* delim)
{
    command_line command;

    int len = strlen(buf);

    if (len > 0 && buf[len - 1] == '\n')
    {
        buf[len - 1] = '\0';
    }

    command.num_token = count_token(buf, delim);

    command.command_list =
        malloc(sizeof(char*) * (command.num_token + 1));

    char *saveptr;
    char *token = strtok_r(buf, delim, &saveptr);

    int i = 0;

    while (token != NULL)
    {
        command.command_list[i] = malloc(strlen(token) + 1);
        strcpy(command.command_list[i], token);

        i++;
        token = strtok_r(NULL, delim, &saveptr);
    }

    command.command_list[i] = NULL;

    return command;
}


void free_command_line(command_line* command)
{
    for (int i = 0; i < command->num_token; i++)
    {
        free(command->command_list[i]);
    }

    free(command->command_list);
}
