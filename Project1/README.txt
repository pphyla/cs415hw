CS 415 Fall 2026
Project 1: Pseudo Shell
Author: Nora Hays
Duck ID: 952094652
Date: 10/23/26

1. DESCRIPTION:

This project implements a single-threaded pseudo-shell for basic file-system
navigation, file manipulation, and file I/O. The shell reads one line at a
time with getline(), splits semicolon-separated commands, validates each
command's arguments, and calls the corresponding implementation. It supports
both interactive mode and file mode.

2. SUPPORTED COMMANDS:

    ls                  - Lists the names in the current directory in dictionary order.
    pwd                 - Prints the shell's current working directory.
    mkdir <name>        - Creates a directory with the specified name.
    cd <directory>      - Changes the current working directory.
    cp <src> <dst>      - Copies one regular file to a file or directory destination.
    mv <src> <dst>      - Copies one regular file to the destination and removes the source.
    rm <filename>       - Removes one regular file.
    cat <filename>      - Writes a file's contents to standard output.
    exit                - Exits the pseudo-shell.

Commands on one input line may be separated with a semicolon, for example:

    cd test; pwd; ls

3. COMPILATION:

Run make in the project directory:

    make

This creates the executable named pseudo-shell. The Makefile compiles with
C11, POSIX 2008 support, and -Wall -Wextra -Werror. Run make clean to remove
the executable, object files, and output.txt.

4. USAGE:

1. Interactive Mode

Start the shell with no command-line arguments:

    ./pseudo-shell

The shell displays the prompt >>> and waits for one input line at a time.
Examples include:

    pwd
    mkdir test; cd test; pwd
    cp ../input.txt .; cat input.txt

Enter exit to quit the shell. Invalid commands and incorrect argument counts
produce an error message, and the rest of that input line is skipped.

2. File Mode

Run the shell with -f followed by an input filename:

    ./pseudo-shell -f input.txt

The shell reads commands from the named file without displaying interactive
prompts. It creates or overwrites output.txt in the current directory and
writes command output there. If the input file cannot be opened, the program
returns a nonzero status without executing commands.
