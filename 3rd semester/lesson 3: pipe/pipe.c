#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BLOCK_SIZE 1024
#define dbgprnt(X) printf("we reached this №%d\n", X);

int main (int argc, char* argv[])
{
int pipefd[2] = {0};
pid_t pid = 0;
char buf_prnt[BLOCK_SIZE] = {0}, buf_chld[BLOCK_SIZE] = {0};
int ret_val = 0, file_desc = -1;
int numb_of_read_symb = 0;

char* prog_name = argv[1];

if (argc != 2)
        {
        printf ("program must have 1 argument\n");
        return 0;
        }

errno = 0;
ret_val = pipe (pipefd);
if (ret_val != 0 )
        {
        perror ("pipe");
        return 0;
        }

// dbgprnt(1);

errno = 0;
pid = fork ();

if (pid < 0)
        {
        perror ("fork");
        return 0;
        }
else if (pid == 0)
        {
        // dbgprnt(2);
        close (pipefd[0]);

        errno = 0;
        file_desc = open (prog_name, O_RDONLY);
        if (file_desc  == -1)
                {
                perror ("open");
                exit (EXIT_FAILURE);
                }

        while ((numb_of_read_symb = read (file_desc, &buf_chld, BLOCK_SIZE)) > 0)
                {
                // dbgprnt(BLOCK_SIZE);
                write (pipefd[1], &buf_chld, numb_of_read_symb);
                }

        close (pipefd[1]);
        exit (EXIT_SUCCESS);
        }
else
        {
        // dbgprnt(3);
        close (pipefd[1]);
        
        while ((numb_of_read_symb = read (pipefd[0], &buf_prnt, BLOCK_SIZE)) > 0)
                {
                // dbgprnt(2*BLOCK_SIZE);
                write (STDOUT_FILENO, &buf_prnt, numb_of_read_symb);
                }

        close (pipefd[0]);
        wait (NULL);
        // dbgprnt(4);
        exit (EXIT_SUCCESS);
        }

return 0;
}
