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


#define NUMB_OF_CHARS   1024
#define SHM_SIZE        ((sizeof(char))*NUMB_OF_CHARS)
#define FTOK_PATH       "/home/speedstalker/.vimrc"

#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)


int main (int argc, char* argv[])
{
key_t sem_key = 0, shm_key = 0;
char  *file_name = NULL, *shm_char_p = NULL;
int   ret_val = 0;
int   numb_of_read_bytes = 0, file_read_desc = 0;
int   sem_id = 0, shm_id = 0;

struct sembuf sops[5];


//------------------------------------------------------------------------------
if (argc != 2)
        {
        printf ("Usage: %s [FILE_NAME]\n", argv[0]);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Only one instance of 'sender' should run simultaneously
//------------------------------------------------------------------------------
if ((sem_key = ftok (FTOK_PATH, 'A')) == -1)
        HANDLE_ERROR("ftok sem_key");

if ((sem_id = semget (sem_key, 6, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1)
        HANDLE_ERROR("sem_id semget");

sops[0].sem_num = 0;
sops[0].sem_op  = 0;
sops[0].sem_flg = IPC_NOWAIT;

sops[1].sem_num = 0;
sops[1].sem_op  = 1;
sops[1].sem_flg = SEM_UNDO;

if (semop (sem_id, sops, 2) == -1)
        {
        if (errno == EAGAIN)    // if not => one sender is already running
                {               // and second sender tries to enter
                printf ("You can not have more than one sender!\n");
                exit (EXIT_FAILURE);
                }
        else
                HANDLE_ERROR("one instance of sender semop");
        }
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
file_name = argv[1];

if ((file_read_desc = open (file_name, O_RDONLY)) == -1)
        HANDLE_ERROR("file open");
//------------------------------------------------------------------------------
if ((shm_key = ftok (FTOK_PATH, 'Z')) == -1)
        HANDLE_ERROR("ftok shm_key");

if ((shm_id = shmget (shm_key, SHM_SIZE, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1)
        if (errno != EEXIST)
                HANDLE_ERROR("shmget");

if ((shm_char_p = (char*)shmat (shm_id, NULL, 0)) == (char*)(-1))
        HANDLE_ERROR("shmat");
//------------------------------------------------------------------------------
// Clear 2nd(r/w) semaphore if its value != 0 due to last launches of programs
//------------------------------------------------------------------------------
ret_val = 0;
while (ret_val == 0)
        {
        sops[0].sem_num =  2;
        sops[0].sem_op  = -1;
        sops[0].sem_flg = IPC_NOWAIT;

        if ((ret_val = semop (sem_id, sops, 1)))
                if (errno != EAGAIN)
                        HANDLE_ERROR("clear sender's r/w sem semop");
        }
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Wait for partner
//------------------------------------------------------------------------------
sops[0].sem_num = 5;    // increase partner's sem...
sops[0].sem_op  = 1;
sops[0].sem_flg = SEM_UNDO;

if (semop (sem_id, sops, 1))
        HANDLE_ERROR("waiting, increase partner's sem semop");
//------------------------------------------------------------------------------
sops[0].sem_num =  5;   // check if partner pass his semop and exited between my two semops
sops[0].sem_op  = -1;   // if so, delete sem_id to avoid useless waiting
sops[0].sem_flg = IPC_NOWAIT;   // by exiting with error from program

sops[1].sem_num =  5;
sops[1].sem_op  =  1;
sops[1].sem_flg =  0;

sops[2].sem_num =  4;   // wait for partner to increase my sem
sops[2].sem_op  = -1;
sops[2].sem_flg =  0;

sops[3].sem_num =  4;
sops[3].sem_op  =  1;
sops[3].sem_flg = SEM_UNDO;

if (semop (sem_id, sops, 4))
        {
        if (errno == EAGAIN)
                {
                if (semctl (sem_id, 0, IPC_RMID))
                        HANDLE_ERROR("before goto semctl IPC_RMID");
                goto resource_cleanup;
                }
        else
                HANDLE_ERROR("waiting increase from partner semop");
        }
//------------------------------------------------------------------------------
// Let sender enter the read/write cycle for the first time
//------------------------------------------------------------------------------
sops[0].sem_num = 2;
sops[0].sem_op  = 1;
sops[0].sem_flg = SEM_UNDO;

if (semop (sem_id, sops, 1))
        HANDLE_ERROR("let sender enter semop");
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// R/W cycle
//------------------------------------------------------------------------------
numb_of_read_bytes = 1;
while (numb_of_read_bytes > 0)
        {
        sops[0].sem_num =  1;   // alive sem
        sops[0].sem_op  = -1;
        sops[0].sem_flg = IPC_NOWAIT;

        sops[1].sem_num =  1;
        sops[1].sem_op  =  1;
        sops[1].sem_flg =  0;

        sops[2].sem_num =  5;   // wait sem
        sops[2].sem_op  = -1;
        sops[2].sem_flg = IPC_NOWAIT;

        sops[3].sem_num =  5;
        sops[3].sem_op  =  1;
        sops[3].sem_flg =  0;

        sops[4].sem_num =  2;   // r/w sem
        sops[4].sem_op  = -1;
        sops[4].sem_flg =  0;

        if (semop (sem_id, sops, 5))
                {
                if (errno == EAGAIN)
                        break;  // printf ("\nPartner exited.\n");
                else
                        HANDLE_ERROR("sender's turn semop");
                }


        if ((numb_of_read_bytes = read (file_read_desc, shm_char_p, NUMB_OF_CHARS - 1)) == -1)
                HANDLE_ERROR("read");
        shm_char_p[numb_of_read_bytes] = '\0';


        sops[0].sem_num =  1;   // alive sem
        sops[0].sem_op  = -1;
        sops[0].sem_flg = IPC_NOWAIT;

        sops[1].sem_num =  1;
        sops[1].sem_op  =  1;
        sops[1].sem_flg =  0;

        sops[2].sem_num =  5;   // wait sem
        sops[2].sem_op  = -1;
        sops[2].sem_flg = IPC_NOWAIT;

        sops[3].sem_num =  5;
        sops[3].sem_op  =  1;
        sops[3].sem_flg =  0;

        sops[4].sem_num =  3;   // r/w sem
        sops[4].sem_op  =  1;
        sops[4].sem_flg =  0;   //SEM_UNDO;

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

if (close (file_read_desc))
        HANDLE_ERROR("file close");
//------------------------------------------------------------------------------

exit (EXIT_SUCCESS);
}
