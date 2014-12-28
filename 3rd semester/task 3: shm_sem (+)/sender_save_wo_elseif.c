#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <fcntl.h>


#define SHM_SIZE        ((sizeof(char))*1024)
#define FTOK_PATH       "/home/speedstalker/.vimrc"

#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)


int main (int argc, char* argv[])
{
key_t snd_sem_key = 0, shm_key = 0, shm_sem_key = 0;
char  *file_name = NULL, *shm_char_p = NULL;
int   file_read_desc = 0;
int   snd_sem_id = 0, shm_id = 0, shm_sem_id = 0;

struct sembuf sops[2];


//-----
if (argc != 2)
        {
        printf ("Usage: %s [FILE_NAME]\n", argv[0]);
        exit (EXIT_FAILURE);
        }
//-----


//-----
if ((snd_sem_key = ftok (FTOK_PATH, 'A')) == -1)
        HANDLE_ERROR("ftok snd_sem_key");

errno = 0;
snd_sem_id = semget (snd_sem_key, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
if (snd_sem_id >= 0)
        {
        sops[0].sem_num = 0;
        sops[0].sem_op  = 1;
        sops[0].sem_flg = 0;

        sops[1].sem_num =  0;
        sops[1].sem_op  = -1;
        sops[1].sem_flg = SEM_UNDO;

        if (semop (snd_sem_id, sops, 2))
                HANDLE_ERROR("semop init");
        }
else
        {
        if (errno == EEXIST)
                {
                snd_sem_id = semget (snd_sem_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);

                sops[0].sem_num =  0;
                sops[0].sem_op  = -1;
                sops[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

                if (semop (snd_sem_id, sops, 1) == -1)
                        {
                        if (errno == EAGAIN)
                                {
                                printf ("You can not have more than one sender!\n");
                                exit (EXIT_FAILURE);
                                }
                        else
                                HANDLE_ERROR("semop after EEXIST");
                        }
                }
        else
                HANDLE_ERROR("snd semget");
        }
//-----

//-----
file_name = argv[1];

if ((file_read_desc = open (file_name, O_RDONLY)) == -1)
        HANDLE_ERROR("file open");
//-----
if ((shm_key = ftok (FTOK_PATH, 'Z')) == -1)
        HANDLE_ERROR("ftok shm_key");

if ((shm_id = shmget (shm_key, SHM_SIZE, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1)
        if (errno != EEXIST)
                HANDLE_ERROR("shmget");

if ((shm_char_p = (char*)shmat (shm_id, NULL, 0)) == (char*)(-1))
        HANDLE_ERROR("shmat");
//-----
if ((shm_sem_key = ftok (FTOK_PATH, 'Y')) == -1)
        HANDLE_ERROR("ftok shm_sem_key");

if ((shm_sem_id = semget (shm_sem_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1)
        if (errno == EEXIST)

        else
                HANDLE_ERROR("shm semget");

sops[0].sem_num = 0;
sops[0].sem_op  = 1;
sops[0].sem_flg = 0;

sops[1].sem_num =  0;
sops[1].sem_op  = -1;
sops[1].sem_flg = SEM_UNDO;

if (semop (shm_sem_id, sops, 2))
        HANDLE_ERROR("shm semop");



//-----
if (semctl (snd_sem_id, 0, IPC_RMID))
        HANDLE_ERROR("semctl IPC_RMID");

exit (EXIT_SUCCESS);
}
