#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

int main (int argc, char *argv[])
{
int i = 0, base = 10, status = 0;
char *endptr = NULL, *str = NULL;
long numb_of_ch = 0, child_numb = 0, pid = 0, ppid = 0;

if (argc != 2)
	{
	exit(EXIT_FAILURE);
	}
           
str = argv [1];

errno = 0;
numb_of_ch = strtol (str, &endptr, base);

if ((errno == ERANGE && (numb_of_ch == LONG_MAX || numb_of_ch == LONG_MIN)) || (errno != 0 && numb_of_ch == 0))
	{
	perror ("strtol");
	exit (EXIT_FAILURE);
	}

if (endptr == str)
	{
	fprintf (stderr, "No digits were found\n");
	exit (EXIT_FAILURE);
	}

// -----

for (i = 0; i < numb_of_ch; i++)
	{
	child_numb++;
	pid = fork ();
	
	if (pid < 0)
		{
		assert (!"pid < 0");
		}
	else if (pid == 0)
		{
		pid  = getpid  ();
		ppid = getppid ();
		printf ("%ld. %ld, %ld\n", child_numb, pid, ppid);
		exit (EXIT_SUCCESS);
		}
	else
		{
		pid = wait (&status);
		}
	}

exit (EXIT_SUCCESS);
}

