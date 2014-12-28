#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <sys/ipc.h>
#include <sys/msg.h>


#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main (int argc, char *argv[])
{
int i = 0, base = 10;
char *endptr = NULL, *str = NULL;
long numb_of_ch = 0, child_numb = 0, killed = 0;
pid_t pid = 0;

int msqid = 0;
struct msgsnd_msg
        {
        long type;
        } message, buf;

if (argc != 2)
	{
        printf ("One argument should be given.\n");
	exit (EXIT_FAILURE);
	}

str = argv [1];
errno = 0;
numb_of_ch = strtol (str, &endptr, base);

if ((errno == ERANGE && (numb_of_ch == LONG_MAX || numb_of_ch == LONG_MIN)) || (errno != 0 && numb_of_ch == 0))
        HANDLE_ERROR("strtol");

if (endptr == str)
	{
	fprintf (stderr, "No digits were found\n");
	exit (EXIT_FAILURE);
	}

if (numb_of_ch <= 0)
        {
        printf ("Enter positive number.\n");
        exit (EXIT_FAILURE);
        }

// -----

if ((msqid = msgget (IPC_PRIVATE, O_CREAT | O_EXCL | (S_IRUSR | S_IWUSR))) == -1)
        HANDLE_ERROR("msgget");

child_numb = 1;
message.type = child_numb;
if ((msgsnd (msqid, &message, sizeof(message) - sizeof(message.type), IPC_NOWAIT)) == -1)
        HANDLE_ERROR("msgsnd");
child_numb = 0;

for (i = 0; i < numb_of_ch; i++)
	{
	child_numb++;
	pid = fork ();

	if (pid < 0)
                HANDLE_ERROR("fork");
        else if (pid == 0)
                {
                if ((msgrcv (msqid, &buf, sizeof(buf) - sizeof(buf.type), child_numb, 0)) == -1)
                        HANDLE_ERROR("msgrcv");

                printf ("numb: %ld, pid: %d\n", buf.type, getpid());
                sleep(10);
                /*
                if ((fflush (NULL)) != 0)
                        HANDLE_ERROR("fflush");
                */

                message.type = child_numb + 1;
                if ((msgsnd (msqid, &message, sizeof(message) - sizeof(message.type), 0)) == -1)
                        HANDLE_ERROR("msgsnd");

                exit (EXIT_SUCCESS);
                }
        else
                {
                pid = 0;
                pid = waitpid (-1, NULL, WNOHANG);
                if (pid > 0)
                        killed++;
                }
	}

while (killed < numb_of_ch)
        {
        pid = 0;
        pid = waitpid (-1, NULL, WNOHANG);
        if (pid > 0)
                killed++;
        }

/*
child_numb = numb_of_ch + 1;
if ((msgrcv (msqid, &buf, sizeof(buf) - sizeof(buf.type), child_numb, 0)) == -1)
        HANDLE_ERROR("msgrcv");
*/

if ((msgctl (msqid, IPC_RMID, NULL)) != 0)
        HANDLE_ERROR("msgctl");

exit (EXIT_SUCCESS);
}
