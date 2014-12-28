#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


int main ()
{
int ret_val = 0;
int fd[2] = {0};
char buf[] = "acyc", buf1[sizeof (buf)] = {1};

pipe (fd);
write (fd[1], buf, sizeof (buf));
close (fd[1]);

errno = 0;
ret_val = read (fd[0], buf1, sizeof (buf));
perror ("read");
printf ("ret_val = %d, buf1[0] = %d, buf1 = %s\n", ret_val, buf1[0], buf1);
write (STDOUT_FILENO, buf1, sizeof (buf));
fflush (NULL);
close (fd[0]);


exit (EXIT_SUCCESS);
}
