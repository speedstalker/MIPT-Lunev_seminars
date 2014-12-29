#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

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
key_t snd_sem_key = 0, shm_key = 0, shm_alive_sem_key = 0;
char  *file_name = NULL, *shm_char_p = NULL;
int   ret_val = 0;
int   numb_of_read_bytes = 0, file_read_desc = 0;
int   snd_sem_id = 0, shm_id = 0, shm_alive_sem_id = 0;

struct sembuf sops[3];


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
if ((snd_sem_key = ftok (FTOK_PATH, 'A')) == -1)
        HANDLE_ERROR("ftok snd_sem_key");

snd_sem_id = semget (snd_sem_key, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
if (snd_sem_id >= 0)      // this set was created first time
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

else if (errno == EEXIST)       // the set was somehow created before we came here
        {
        snd_sem_id = semget (snd_sem_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);

        sops[0].sem_num =  0;   // if we can substract 1 from sem, then do it
        sops[0].sem_op  = -1;   // cause this sem left by the previous exited sender
        sops[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

        if (semop (snd_sem_id, sops, 1) == -1)
                {
                if (errno == EAGAIN)    // if not => one sender is already running
                        {               // and second sender tries to enter
                        printf ("You can not have more than one sender!\n");
                        exit (EXIT_FAILURE);
                        }
                else
                        HANDLE_ERROR("semop after EEXIST");
                }
        }

else
        HANDLE_ERROR("snd semget");
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
// Description of semaphores in this set:
//
// sem_num:     0 - sender    use it to control it's read/write operations
//              1 - receiver  use it to control it's read/write operations
//                      (0 - blocked, 1 - free)
//
//              2 - is  sender    alive/dead?
//              3 - is  receiver  alive/dead?
//                      (0 - alive, 1 - dead)
//
//              4 - is sender    inside or below r/w cycle
//              5 - is receiver  inside or below r/w cycle
//                      (0 - yes, 1 - not yet)
//
//              6 - is sender    ready to start r/w cycle
//              7 - is receiver  ready to start r/w cycle
//                      (0 - ready, 1 - not yet)
//------------------------------------------------------------------------------
if ((shm_alive_sem_key = ftok (FTOK_PATH, 'X')) == -1)
        HANDLE_ERROR("ftok shm_alive_sem_key");

shm_alive_sem_id = semget (shm_alive_sem_key, 8, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
if (shm_alive_sem_id >= 0)      // sender created set first time and got it first
        {
        sops[0].sem_num = 2;
        sops[0].sem_op  = 1;
        sops[0].sem_flg = 0;

        sops[1].sem_num =  2;
        sops[1].sem_op  = -1;
        sops[1].sem_flg = SEM_UNDO;

        if (semop (shm_alive_sem_id, sops, 2))
                HANDLE_ERROR("shm_alive_sem init semop");

        sops[0].sem_num = 4;
        sops[0].sem_op  = 1;
        sops[0].sem_flg = 0;

        sops[1].sem_num = 5;
        sops[1].sem_op  = 1;
        sops[1].sem_flg = 0;

        if (semop (shm_alive_sem_id, sops, 2))
                HANDLE_ERROR("shm_alive_sem init in_cycle? semop");

        sops[0].sem_num = 6;
        sops[0].sem_op  = 1;
        sops[0].sem_flg = 0;

        sops[1].sem_num = 7;
        sops[1].sem_op  = 1;
        sops[1].sem_flg = 0;

        if (semop (shm_alive_sem_id, sops, 2))
                HANDLE_ERROR("shm_alive_sem init ready semop");
        }

else if (errno == EEXIST)       // the set was somehow created before we came here
        {
        shm_alive_sem_id = semget (shm_alive_sem_key, 8, IPC_CREAT | S_IRUSR | S_IWUSR);

        sops[0].sem_num = 2;
        sops[0].sem_op  = 0;
        sops[0].sem_flg = IPC_NOWAIT;   // if 2nd sem = 0, sender has never been launched before,
                                        // maybe receiver created set before sender
        if (semop (shm_alive_sem_id, sops, 1) == 0)
                {                       // so, do the same thing as before
                sops[0].sem_num = 2;
                sops[0].sem_op  = 1;
                sops[0].sem_flg = 0;

                sops[1].sem_num =  2;
                sops[1].sem_op  = -1;
                sops[1].sem_flg = SEM_UNDO;

                if (semop (shm_alive_sem_id, sops, 2))
                        HANDLE_ERROR("shm_alive_sem semop after EEXIST");
                }
        else if (errno == EAGAIN)       // sender has been launched before
                {
                sops[0].sem_num =  2;
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
// Clear 0th(r/w) semaphore if its value != 0 due to last launches of programs
//------------------------------------------------------------------------------
ret_val = 0;
while (ret_val == 0)
        {
        sops[0].sem_num =  0;
        sops[0].sem_op  = -1;
        sops[0].sem_flg = IPC_NOWAIT;

        if ((ret_val = semop (shm_alive_sem_id, sops, 1)))
                if (errno != EAGAIN)
                        HANDLE_ERROR("shm_alive_sem semop clear sender's sem");
        }
//------------------------------------------------------------------------------
// Let sender enter the read/write cycle for the first time
//------------------------------------------------------------------------------
sops[0].sem_num = 0;
sops[0].sem_op  = 1;
sops[0].sem_flg = 0;

if (semop (shm_alive_sem_id, sops, 1))
        HANDLE_ERROR("shm_alive_sem semop init before read/write");
//------------------------------------------------------------------------------
sops[0].sem_num =  7;   // decrease partner's semaphore til 0...
sops[0].sem_op  = -1;
sops[0].sem_flg = SEM_UNDO;

if (semop (shm_alive_sem_id, sops, 1))
        HANDLE_ERROR("shm_alive_sem_id decrease before r/w semop");
//------------------------------------------------------------------------------
sops[0].sem_num = 6;    // ... and wait for partner to do the same with my sem
sops[0].sem_op  = 0;
sops[0].sem_flg = 0;

sops[1].sem_num =  4;    // decrease my semaphore to show that I'm in r/w cycle
sops[1].sem_op  = -1;
sops[1].sem_flg = SEM_UNDO;

if (semop (shm_alive_sem_id, sops, 2))
        HANDLE_ERROR("shm_alive_sem_id decrease and r/w semop");
//------------------------------------------------------------------------------
sops[0].sem_num = 7;    // increase partner's sem to avoid continuing r/w cycle,
sops[0].sem_op  = 1;    // after relaunch of partner, when I'm reading/writing
sops[0].sem_flg = SEM_UNDO;

if (semop (shm_alive_sem_id, sops, 1))
        HANDLE_ERROR("shm_alive_sem_id increase before r/w semop");
//------------------------------------------------------------------------------
// W/R cycle
//------------------------------------------------------------------------------
numb_of_read_bytes = 1;
while (numb_of_read_bytes > 0)
        {
        sops[0].sem_num = 3;
        sops[0].sem_op  = 0;
        sops[0].sem_flg = IPC_NOWAIT;

        sops[1].sem_num = 5;
        sops[1].sem_op  = 0;
        sops[1].sem_flg = IPC_NOWAIT;

        sops[2].sem_num =  0;
        sops[2].sem_op  = -1;
        sops[2].sem_flg =  0;

        if (semop (shm_alive_sem_id, sops, 3))
                {
                if (errno == EAGAIN)
                        break;  // printf ("\nPartner exited.\n");
                else
                        HANDLE_ERROR("snd read semop");
                }

        if ((numb_of_read_bytes = read (file_read_desc, shm_char_p, NUMB_OF_CHARS - 1)) == -1)
                HANDLE_ERROR("read");
        shm_char_p[numb_of_read_bytes] = '\0';

        sops[0].sem_num = 3;
        sops[0].sem_op  = 0;
        sops[0].sem_flg = IPC_NOWAIT;

        sops[1].sem_num = 5;
        sops[1].sem_op  = 0;
        sops[1].sem_flg = IPC_NOWAIT;

        sops[2].sem_num = 1;
        sops[2].sem_op  = 1;
        sops[2].sem_flg = 0;

        if (semop (shm_alive_sem_id, sops, 3))
                {
                if (errno == EAGAIN)
                        break;  // printf ("\nPartner exited.\n");
                else
                        HANDLE_ERROR("snd write semop");
                }
        }
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
if (shmdt (shm_char_p) == -1)
        HANDLE_ERROR("shmdt");

if (close (file_read_desc))
        HANDLE_ERROR("file close");
//------------------------------------------------------------------------------

exit (EXIT_SUCCESS);
}
