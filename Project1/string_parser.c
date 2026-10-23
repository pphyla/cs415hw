#include "string_parser.h"

#include <stdlib.h>
#include <string.h>

command_line *parse_command_line(char *text)
{
    char *token;
    char *save_pointer;
    command_line *parsed;

    parsed = malloc(sizeof(*parsed));
    if (parsed == NULL) {
        return NULL;
    }

    parsed->command = NULL;
    parsed->arguments = NULL;
    parsed->argument_count = 0;

    token = strtok_r(text, " \t\r\n", &save_pointer);
    if (token == NULL) {
        return parsed;
    }

    parsed->command = malloc(strlen(token) + 1);
    if (parsed->command == NULL) {
        free(parsed);
        return NULL;
    }
    strcpy(parsed->command, token);

    while ((token = strtok_r(NULL, " \t\r\n", &save_pointer)) != NULL) {
        char **new_arguments;

        new_arguments = realloc(parsed->arguments,
                                sizeof(*parsed->arguments) *
                                (size_t)(parsed->argument_count + 1));
        if (new_arguments == NULL) {
            free_command_line(parsed);
            return NULL;
        }
        parsed->arguments = new_arguments;
        parsed->arguments[parsed->argument_count] = token;
        parsed->argument_count++;
    }

    return parsed;
}

void free_command_line(command_line *parsed)
{
    if (parsed == NULL) {
        return;
    }
    free(parsed->command);
    free(parsed->arguments);
    free(parsed);
}
