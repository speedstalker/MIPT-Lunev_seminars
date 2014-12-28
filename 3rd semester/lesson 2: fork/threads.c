#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <sys/types.h>
#include <unistd.h>

#include <malloc.h>
#include <pthread.h>

#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void* thread_start(void *th_numb);

int main (int argc, char *argv[])
{
int i = 0, base = 10;
char *endptr = NULL, *str = NULL;
long numb_of_th = 0;
pthread_t* th_ids = NULL;

if (argc != 2)
	{
        printf ("One argument should be given.\n");
	exit (EXIT_FAILURE);
	}
           
str = argv [1];
errno = 0;
numb_of_th = strtol (str, &endptr, base);

if ((errno == ERANGE && (numb_of_th == LONG_MAX || numb_of_th == LONG_MIN)) || (errno != 0 && numb_of_th == 0))
        HANDLE_ERROR("strtol");

if (endptr == str)
	{
	fprintf (stderr, "No digits were found\n");
	exit (EXIT_FAILURE);
	}

if (numb_of_th <= 0)
        {
        printf ("Enter positive number.\n");
        exit (EXIT_FAILURE);
        }

// -----

if ((th_ids = (pthread_t*) calloc (numb_of_th, sizeof(pthread_t))) == NULL)
        HANDLE_ERROR("calloc");

for (i = 1; i <= numb_of_th; i++)
	{
	if (pthread_create (&th_ids[i-1], NULL, thread_start, (int*)i) != 0)
                HANDLE_ERROR("pthread_create");
	}

for (i = 0; i < numb_of_th; i++)
        if (pthread_join (th_ids[i], NULL) != 0)
                HANDLE_ERROR("phthread_join");

exit (EXIT_SUCCESS);
}

static void* thread_start(void* th_numb)
	{
	printf ("numb: %d,\t pid: %d, ppid: %d\n", (int)th_numb, getpid(), getppid());
        return 0;
	}
