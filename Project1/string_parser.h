#ifndef STRING_PARSER_H
#define STRING_PARSER_H

typedef struct command_line {
    char *command;
    char **arguments;
    int argument_count;
} command_line;

command_line *parse_command_line(char *text);
void free_command_line(command_line *parsed);

#endif
