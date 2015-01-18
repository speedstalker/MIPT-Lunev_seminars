#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>


#define NUMB_OF_CHARS   1024
#define SHM_SIZE        ((sizeof(char))*NUMB_OF_CHARS)
#define FTOK_PATH       "/home/speedstalker/.vimrc"

#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)


int main (int argc, char* argv[])
{
key_t sem_key = 0, shm_key = 0;
char  *shm_char_p = NULL;
int   numb_of_written_bytes = 0;
int   sem_id = 0, shm_id = 0;

struct sembuf sops[5];


//------------------------------------------------------------------------------
if (argc != 1)
        {
        printf ("Program shouldn't have arguments\n");
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Only one instance of 'receiver' should run simultaneously
//------------------------------------------------------------------------------
if ((sem_key = ftok (FTOK_PATH, 'A')) == -1)
        HANDLE_ERROR("ftok sem_key");

if ((sem_id = semget (sem_key, 6, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1)
        HANDLE_ERROR("sem_id semget");

sops[0].sem_num = 1;
sops[0].sem_op  = 0;
sops[0].sem_flg = IPC_NOWAIT;

sops[1].sem_num = 1;
sops[1].sem_op  = 1;
sops[1].sem_flg = SEM_UNDO;

if (semop (sem_id, sops, 2) == -1)
        {
        if (errno == EAGAIN)    // if not => one receiver is already running
                {               // and second receiver tries to enter
                printf ("You can not have more than one receiver!\n");
                exit (EXIT_FAILURE);
                }
        else
                HANDLE_ERROR("one instance of receiver semop");
        }
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
if ((shm_key = ftok (FTOK_PATH, 'Z')) == -1)
        HANDLE_ERROR("ftok shm_key");

if ((shm_id = shmget (shm_key, SHM_SIZE, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1)
        if (errno != EEXIST)
                HANDLE_ERROR("shmget");

if ((shm_char_p = (char*)shmat (shm_id, NULL, SHM_RDONLY)) == (char*)(-1))
        HANDLE_ERROR("shmat");
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Wait for partner
//------------------------------------------------------------------------------
sops[0].sem_num = 4;    // increase partner's sem...
sops[0].sem_op  = 1;
sops[0].sem_flg = SEM_UNDO;

if (semop (sem_id, sops, 1))
        HANDLE_ERROR("waiting, increase partner's sem semop");
//------------------------------------------------------------------------------
sops[0].sem_num =  4;   // check if partner pass his semop and exited between my two semops
sops[0].sem_op  = -1;   // if so, delete sem_id to avoid useless waiting
sops[0].sem_flg = IPC_NOWAIT;   // by exiting with error from program

sops[1].sem_num =  4;
sops[1].sem_op  =  1;
sops[1].sem_flg =  0;

sops[2].sem_num =  5;   // wait for partner to increase my sem
sops[2].sem_op  = -1;
sops[2].sem_flg =  0;

sops[3].sem_num =  5;
sops[3].sem_op  =  1;
sops[3].sem_flg = SEM_UNDO;

if (semop (sem_id, sops, 4))
        {
        if (errno == EAGAIN)
                {
                goto resource_cleanup;
                }
        else
                HANDLE_ERROR("waiting increase from partner semop");
        }
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// R/W cycle
//------------------------------------------------------------------------------
numb_of_written_bytes = 1;
while (numb_of_written_bytes > 0)
        {
        sops[0].sem_num =  0;   // alive sem
        sops[0].sem_op  = -1;
        sops[0].sem_flg = IPC_NOWAIT;

        sops[1].sem_num =  0;
        sops[1].sem_op  =  1;
        sops[1].sem_flg =  0;

        sops[2].sem_num =  4;   // wait sem
        sops[2].sem_op  = -1;
        sops[2].sem_flg = IPC_NOWAIT;

        sops[3].sem_num =  4;
        sops[3].sem_op  =  1;
        sops[3].sem_flg =  0;

        sops[4].sem_num =  3;   // r/w sem
        sops[4].sem_op  = -1;
        sops[4].sem_flg =  0;

        if (semop (sem_id, sops, 5))
                {
                if (errno == EAGAIN)
                        break;  // printf ("\nPartner exited.\n");
                else
                        HANDLE_ERROR("receiver's turn semop");
                }


        if ((numb_of_written_bytes = printf ("%s", shm_char_p)) < 0)
                HANDLE_ERROR("printf");
        /*
        if (fflush(NULL) == EOF)
                HANDLE_ERROR("fflush");
        */


        sops[0].sem_num =  0;   // alive sem
        sops[0].sem_op  = -1;
        sops[0].sem_flg = IPC_NOWAIT;

        sops[1].sem_num =  0;
        sops[1].sem_op  =  1;
        sops[1].sem_flg =  0;

        sops[2].sem_num =  4;   // wait sem
        sops[2].sem_op  = -1;
        sops[2].sem_flg = IPC_NOWAIT;

        sops[3].sem_num =  4;
        sops[3].sem_op  =  1;
        sops[3].sem_flg =  0;

        sops[4].sem_num =  2;    // r/w sem
        sops[4].sem_op  =  1;
        sops[4].sem_flg = SEM_UNDO;

        if (semop (sem_id, sops, 5))
                {
                if (errno == EAGAIN)
                        break;  // printf ("\nPartner exited.\n");
                else
                        HANDLE_ERROR("give turn to partner semop");
                }
        }
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
resource_cleanup:

if (shmdt (shm_char_p) == -1)
        HANDLE_ERROR("shmdt");
if (shmctl (shm_id, IPC_RMID, NULL))
        HANDLE_ERROR("shmctl RMID");
//------------------------------------------------------------------------------
if (semctl (sem_id, 0, IPC_RMID))
        HANDLE_ERROR("semctl IPC_RMID");
//------------------------------------------------------------------------------

exit (EXIT_SUCCESS);
}
