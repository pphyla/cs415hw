#include<stdio.h>
#include <sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

void script_print (pid_t* pid_ary, int size);

/*
 * CS 415 Lab 4 - Processes and the exec system call
 *
 * Goal: start n copies of "./iobound -seconds 5" at the same time,
 *       then wait for all of them to finish.
 *
 * Usage: ./lab4 <num_processes>
 *
 * WARNING: do NOT test with more than 7 processes until you are 100% sure
 *          your code is correct (a fork bomb can crash your VM).
 *
 * Useful man pages: man 2 fork, man 3 execvp, man 2 waitpid
 */

int main(int argc,char*argv[])
{
	if (argc != 2)
	{
		printf ("Wrong number of arguments\n");
		printf("Input: ./lab4 <num_processes>\n");
		exit (0);
	}

	/*
	 * TODO #0: get the number of processes from the command line.
	 * Hint: argv[1] is a string, use atoi() to convert it to an int.
	 */
	int num_processes = atoi(argv[1]);
	if (num_processes <= 0)
	{
		printf("Number of processes must be positive\n");
		exit (1);
	}


	/*
	 * TODO #1: declare the child process pool.
	 * Hint: you need an array of pid_t with num_processes elements.
	 *       Since the size is only known at runtime, use malloc().
	 */
	pid_t *process_pool = malloc(num_processes * sizeof(pid_t));
	if (process_pool == NULL)
	{
		perror("malloc");
		exit (1);
	}


	/*
	 * TODO #2: spawn num_processes new processes.
	 * Hint: use a for loop. In each iteration call fork() once.
	 *   fork() returns:
	 *     == 0  -> you are in the CHILD process
	 *     >  0  -> you are in the PARENT process, the value is the child's pid
	 *     <  0  -> fork failed (print an error with perror() and exit)
	 *
	 *   In the CHILD:
	 *     - build the argument array for execvp. It must end with NULL:
	 *           char *args[] = {"./iobound", "-seconds", "5", NULL};
	 *     - call execvp(args[0], args);
	 *     - execvp only returns if it FAILED, so right after it call
	 *       perror() and exit(). Otherwise the child would keep running
	 *       the loop and fork its own children!
	 *
	 *   In the PARENT:
	 *     - save the child's pid into process_pool[i].
	 *     - do NOT wait here, or the children will run one at a time
	 *       instead of all at once.
	 */
	for (int i = 0; i < num_processes; i++)
	{
		pid_t child_pid = fork();

		if (child_pid < 0)
		{
			perror("fork");
			free(process_pool);
			exit (1);
		}
		else if (child_pid == 0)
		{
			char *args[] = {"./iobound", "-seconds", "5", NULL};
			execvp(args[0], args);
			perror("execvp");
			exit (1);
		}
		else
		{
			process_pool[i] = child_pid;
		}
	}


	/*
	 * TODO #3: call script_print() with the process pool and its size.
	 * It writes top_script.sh so you can monitor your children with top.
	 * On IX-Dev it prints "top command: : No such file or directory".
	 * Ignore that error, open another terminal and run top instead.
	 */
	script_print(process_pool, num_processes);


	/*
	 * TODO #4: wait for ALL children to finish.
	 * Hint: loop over process_pool and call waitpid(process_pool[i], NULL, 0);
	 */
	for (int i = 0; i < num_processes; i++)
	{
		if (waitpid(process_pool[i], NULL, 0) == -1)
		{
			perror("waitpid");
			free(process_pool);
			exit (1);
		}
	}


	/*
	 * TODO #5: free any dynamic memory you allocated.
	 */
	free(process_pool);


	return 0;
}


/*
 * Provided function - do not modify.
 * Writes top_script.sh ("top -p pid1 -p pid2 ...") and tries to open it
 * in a new gnome-terminal. This does not work on IX-Dev: instead, ssh into
 * IX-Dev from another terminal and run top yourself.
 */
void script_print (pid_t* pid_ary, int size)
{
	FILE* fout;
	fout = fopen ("top_script.sh", "w");
	fprintf(fout, "#!/bin/bash\ntop");
	for (int i = 0; i < size; i++)
	{
		fprintf(fout, " -p %d", (int)(pid_ary[i]));
	}
	fprintf(fout, "\n");
	fclose (fout);

	char* top_arg[] = {"gnome-terminal", "--", "bash", "top_script.sh", NULL};

	pid_t top_pid;

	top_pid = fork();
	{
		if (top_pid == 0)
		{
			if(execvp(top_arg[0], top_arg) == -1)
			{
				perror ("top command: ");
			}
			exit(0);
		}
	}
}
