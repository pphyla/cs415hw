CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L

pseudo-shell: main.o command.o string_parser.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c command.h string_parser.h
	$(CC) $(CFLAGS) -c main.c

command.o: command.c command.h
	$(CC) $(CFLAGS) -c command.c

string_parser.o: string_parser.c string_parser.h
	$(CC) $(CFLAGS) -c string_parser.c

clean:
	rm -f pseudo-shell main.o command.o string_parser.o output.txt
