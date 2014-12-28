#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>


#define BLOCK_SIZE 1024

#define SND_FIFO      "/tmp/snd_test"
#define RCV_FIFO      "/tmp/rcv_test"
#define DATA_FIFO     "/tmp/my_transmitter"
#define END_SYNC_FIFO "/tmp/end_sync"

#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main (int argc, char* argv[])
{
char buf[BLOCK_SIZE] = {0}, rcv_buf[PIPE_BUF] = {0};
int fifo_read_desc = 0;
int snd_read_desc = 0, rcv_write_desc = 0;
ssize_t numb_of_read_bytes = 0;

//-----
if (argc != 1)
        {
        printf ("Program shouldn't have arguments\n");
        exit (EXIT_FAILURE);
        }
//-----



//-----
// maybe just open with O_CREAT flag?
if (mkfifo (SND_FIFO, S_IRUSR | S_IWUSR))
        if (errno != EEXIST)
                HANDLE_ERROR("snd_read mkfifo");

if ((snd_read_desc = open (SND_FIFO, O_RDONLY)) == -1)
        HANDLE_ERROR("snd_read fifo open");
//-----



//-----
if (mkfifo (RCV_FIFO, S_IRUSR | S_IWUSR))
        if (errno != EEXIST)
                HANDLE_ERROR("rcv_write mkfifo");

if ((rcv_write_desc = open (RCV_FIFO, O_WRONLY)) == -1)
        HANDLE_ERROR("rcv_write fifo open");

if (fcntl (rcv_write_desc, F_SETFL, O_NONBLOCK))
        HANDLE_ERROR("fcntl rcv_write nonblock");

if (fcntl (rcv_write_desc, F_SETPIPE_SZ, PIPE_BUF) <= 0)
        HANDLE_ERROR("fcntl rcv_write size");
assert (PIPE_BUF == fcntl (rcv_write_desc, F_GETPIPE_SZ));
//-----
if (write (rcv_write_desc, &rcv_buf, PIPE_BUF) < 0)
        {
        printf ("You can not have more than one receiver!\n");
        if (close (snd_read_desc))
                HANDLE_ERROR("snd_read_desc close");
        if (close (rcv_write_desc))
                HANDLE_ERROR("rcv_write_desc close");
        printf ("Exiting.\n");
        exit (EXIT_FAILURE);
        }
//-----


//-----
if (mkfifo (DATA_FIFO, S_IRUSR | S_IWUSR))
        if (errno != EEXIST)
                HANDLE_ERROR("data mkfifo");

if ((fifo_read_desc = open (DATA_FIFO, O_RDONLY)) == -1)
        HANDLE_ERROR("data fifo open");
//-----
while ((numb_of_read_bytes = read (fifo_read_desc, &buf, BLOCK_SIZE)) > 0)
        {
        if (write (STDOUT_FILENO, &buf, numb_of_read_bytes) < 0)
                HANDLE_ERROR("write");
        }
//-----
if (numb_of_read_bytes < 0)
        HANDLE_ERROR("read");
//-----

if (close (fifo_read_desc))
        HANDLE_ERROR("data fifo close");

if (unlink (DATA_FIFO))
        HANDLE_ERROR("unlink");

if (unlink (RCV_FIFO))
        HANDLE_ERROR("unlink");

if (unlink (SND_FIFO))
        HANDLE_ERROR("unlink");

if (close (rcv_write_desc))
        HANDLE_ERROR("rcv_write_desc close");

//-----

if (read (snd_read_desc, &rcv_buf, PIPE_BUF) < 0)
        HANDLE_ERROR("snd_read_desc read");

if (close (snd_read_desc))
        HANDLE_ERROR("snd_read_desc close");

exit (EXIT_SUCCESS);
}
