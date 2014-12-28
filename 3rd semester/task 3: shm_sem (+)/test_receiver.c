#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>


#define FTOK_PATH       "/home/speedstalker/.vimrc"

#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)


int main (int argc, char* argv[])
{
key_t sem_key = 0;
int   i = 0;
int   sem_id = 0;

struct sembuf sops[2];


//-----
if ((sem_key = ftok (FTOK_PATH, 'M')) == -1)
        HANDLE_ERROR("ftok sem_key");

sem_id = semget (sem_key, 2, IPC_CREAT | S_IRUSR | S_IWUSR);
//-----


//-----
sops[0].sem_num = 0;
sops[0].sem_op  = 0;
sops[0].sem_flg = IPC_NOWAIT;

sops[1].sem_num =  1;
sops[1].sem_op  = -1;
sops[1].sem_flg =  0;

if (semop (sem_id, sops, 2) >= 0)
        printf ("done\n");

else if (errno == EAGAIN)
        printf ("failed\n");

else
        HANDLE_ERROR("semop wait");
//-----


//-----
if (semctl (sem_id, 0, IPC_RMID))
        HANDLE_ERROR("semctl IPC_RMID");

printf ("exiting...\n");
exit (EXIT_SUCCESS);
}
