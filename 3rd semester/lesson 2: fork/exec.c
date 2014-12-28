#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main (int argc, char* argv[])
{
if (argc < 2)
        {
        printf ("Usage: %s prog_name arg1 ... argn\n", argv[0]);
        exit (EXIT_FAILURE);
        }

argv[argc] = NULL;
execvp (argv[1], argv + 1);

// if reached this => execv failed, errno was set
perror ("execvp");
exit (EXIT_FAILURE);
}
