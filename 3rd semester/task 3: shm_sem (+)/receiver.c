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
key_t snd_sem_key = 0, rcv_sem_key = 0, shm_key = 0, shm_alive_sem_key = 0, ready_sem_key = 0;
char  *shm_char_p = NULL;
int   ret_val = 0;
int   numb_of_written_bytes = 0;
int   snd_sem_id = 0, rcv_sem_id = 0, shm_id = 0, shm_alive_sem_id = 0, ready_sem_id = 0;

struct sembuf sops[2];


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
if ((rcv_sem_key = ftok (FTOK_PATH, 'B')) == -1)
        HANDLE_ERROR("ftok rcv_sem_key");

rcv_sem_id = semget (rcv_sem_key, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
if (rcv_sem_id >= 0)      // this set was created first time
        {
        sops[0].sem_num = 0;
        sops[0].sem_op  = 1;
        sops[0].sem_flg = 0;

        sops[1].sem_num =  0;
        sops[1].sem_op  = -1;
        sops[1].sem_flg = SEM_UNDO;

        if (semop (rcv_sem_id, sops, 2))
                HANDLE_ERROR("semop init");
        }

else if (errno == EEXIST)       // the set was somehow created before we came here
        {
        rcv_sem_id = semget (rcv_sem_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);

        sops[0].sem_num =  0;   // if we can substract 1 from sem, then do it
        sops[0].sem_op  = -1;   // cause this sem left by the previous exited receiver
        sops[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

        if (semop (rcv_sem_id, sops, 1) == -1)
                {
                if (errno == EAGAIN)    // if not => one receiver is already running
                        {               // and second receiver tries to enter
                        printf ("You can not have more than one receiver!\n");
                        exit (EXIT_FAILURE);
                        }
                else
                        HANDLE_ERROR("semop after EEXIST");
                }
        }
else
        HANDLE_ERROR("rcv semget");
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
// Description of semaphores in this set:
//
// sem_num:     0 - sender    use it to control it's read/write operations
//              1 - receiver  use it to control it's read/write operations
//                      (0 - blocked, 1 - free)
//
//              2 - is  sender    alive/dead?
//              3 - is  receiver  alive/dead?
//                      (0 - alive, 1 - dead)
//------------------------------------------------------------------------------
if ((shm_alive_sem_key = ftok (FTOK_PATH, 'X')) == -1)
        HANDLE_ERROR("ftok shm_alive_sem_key");

shm_alive_sem_id = semget (shm_alive_sem_key, 4, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
if (shm_alive_sem_id >= 0)      // receiver created set first time and got it first
        {
        sops[0].sem_num = 3;
        sops[0].sem_op  = 1;
        sops[0].sem_flg = 0;

        sops[1].sem_num =  3;
        sops[1].sem_op  = -1;
        sops[1].sem_flg = SEM_UNDO;

        if (semop (shm_alive_sem_id, sops, 2))
                HANDLE_ERROR("shm_alive_sem init semop");
        }

else if (errno == EEXIST)       // the set was somehow created before we came here
        {
        shm_alive_sem_id = semget (shm_alive_sem_key, 4, IPC_CREAT | S_IRUSR | S_IWUSR);

        sops[0].sem_num = 3;
        sops[0].sem_op  = 0;
        sops[0].sem_flg = IPC_NOWAIT;   // if 3rd sem = 0, receiver has never been launched before,
                                        // maybe sender created set before receiver
        if (semop (shm_alive_sem_id, sops, 1) == 0)
                {                       // so, do the same thing as before
                sops[0].sem_num = 3;
                sops[0].sem_op  = 1;
                sops[0].sem_flg = 0;

                sops[1].sem_num =  3;
                sops[1].sem_op  = -1;
                sops[1].sem_flg = SEM_UNDO;

                if (semop (shm_alive_sem_id, sops, 2))
                        HANDLE_ERROR("shm_alive_sem semop after EEXIST");
                }
        else if (errno == EAGAIN)       // receiver has been launched before
                {
                sops[0].sem_num =  3;
                sops[0].sem_op  = -1;
                sops[0].sem_flg = SEM_UNDO;

                if (semop (shm_alive_sem_id, sops, 1))
                        HANDLE_ERROR("shm_alive_sem semop after EAGAIN");
                }
        else
                HANDLE_ERROR("shm_alive_sem semop after EEXIST");
        }

else
        HANDLE_ERROR("shm_alive_sem semget");
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Clear 1th(r/w) semaphore if its value != 0 due to last launches of programs
//------------------------------------------------------------------------------
ret_val = 0;
while (ret_val == 0)
        {
        sops[0].sem_num =  1;
        sops[0].sem_op  = -1;
        sops[0].sem_flg = IPC_NOWAIT;

        if ((ret_val = semop (shm_alive_sem_id, sops, 1)))
                if (errno != EAGAIN)
                        HANDLE_ERROR("shm_alive_sem semop clear receiver's sem");
        }
//------------------------------------------------------------------------------
// Waiting for the partner before r/w
//------------------------------------------------------------------------------
// Description of semaphores in this set:
//
// sem_num:     0 - is sender    ready to start r/w cycle
//              1 - is receiver  ready to start r/w cycle
//                      (0 - ready, 1 - not yet)
//------------------------------------------------------------------------------
if ((ready_sem_key = ftok (FTOK_PATH, 'W')) == -1)
        HANDLE_ERROR("ftok ready_sem_key");

ready_sem_id = semget (ready_sem_key, 2, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
if (ready_sem_id >= 0)      // receiver created set first time and got it first
        {
        sops[0].sem_num = 0;
        sops[0].sem_op  = 1;
        sops[0].sem_flg = 0;

        sops[1].sem_num = 1;
        sops[1].sem_op  = 1;
        sops[1].sem_flg = 0;

        if (semop (ready_sem_id, sops, 2))
                HANDLE_ERROR("ready_sem init semop");
        }

else if (errno == EEXIST)       // the set had been created somehow, don't worry,
        {                       // just get it, everything will be OK
        if ((ready_sem_id = semget (ready_sem_key, 2, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1)
                HANDLE_ERROR("ready_sem EEXIST semget");
        }

else
        HANDLE_ERROR("ready_sem semget");
//------------------------------------------------------------------------------
sops[0].sem_num =  1;   // decrease my semaphore til 0...
sops[0].sem_op  = -1;
sops[0].sem_flg = SEM_UNDO;

if (semop (ready_sem_id, sops, 1))
        HANDLE_ERROR("ready_sem decrease before r/w semop");
//------------------------------------------------------------------------------
sops[0].sem_num = 0;    // ... and wait for partner to do the same
sops[0].sem_op  = 0;
sops[0].sem_flg = 0;

if (semop (ready_sem_id, sops, 1))
        HANDLE_ERROR("ready_sem wait before r/w semop");
//------------------------------------------------------------------------------
// W/R cycle
//------------------------------------------------------------------------------
numb_of_written_bytes = 1;
while (numb_of_written_bytes > 0)
        {
        sops[0].sem_num = 2;
        sops[0].sem_op  = 0;
        sops[0].sem_flg = IPC_NOWAIT;

        sops[1].sem_num =  1;
        sops[1].sem_op  = -1;
        sops[1].sem_flg =  0;

        if (semop (shm_alive_sem_id, sops, 2))
                {
                if (errno == EAGAIN)
                        break;  // printf ("\nPartner exited.\n");
                else
                        HANDLE_ERROR("snd read semop");
                }

        if ((numb_of_written_bytes = printf ("%s", shm_char_p)) < 0)
                HANDLE_ERROR("printf");

        /*
        if (fflush(NULL) == EOF)
                HANDLE_ERROR("fflush");
        */

        sops[0].sem_num = 0;
        sops[0].sem_op  = 1;
        sops[0].sem_flg = 0;

        if (semop (shm_alive_sem_id, sops, 1))
                HANDLE_ERROR("rcv write semop");
        }
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
if (semctl (ready_sem_id, 0, IPC_RMID))
        HANDLE_ERROR("ready_sem semctl IPC_RMID");
//------------------------------------------------------------------------------
if (semctl (shm_alive_sem_id, 0, IPC_RMID))
        HANDLE_ERROR("shm_alive semctl IPC_RMID");
//------------------------------------------------------------------------------
if (shmdt (shm_char_p) == -1)
        HANDLE_ERROR("shmdt");
if (shmctl (shm_id, IPC_RMID, NULL))
        HANDLE_ERROR("shmctl RMID");
//------------------------------------------------------------------------------
if ((snd_sem_key = ftok (FTOK_PATH, 'A')) == -1)
        HANDLE_ERROR("ftok snd_sem_key");
if ((snd_sem_id = semget (snd_sem_key, 1, S_IRUSR | S_IWUSR)) == -1)
        HANDLE_ERROR("snd semget");
if (semctl (snd_sem_id, 0, IPC_RMID))
        HANDLE_ERROR("snd semctl IPC_RMID");
//------------------------------------------------------------------------------
if (semctl (rcv_sem_id, 0, IPC_RMID))
        HANDLE_ERROR("rcv semctl IPC_RMID");
//------------------------------------------------------------------------------

exit (EXIT_SUCCESS);
}
