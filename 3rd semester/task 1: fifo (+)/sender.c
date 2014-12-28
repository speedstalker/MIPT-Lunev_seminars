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
char* file_name = NULL, buf[BLOCK_SIZE] = {0}, snd_buf[PIPE_BUF] = {0};
int file_read_desc = 0, fifo_write_desc = 0;
int snd_write_desc = 0, rcv_read_desc = 0;
ssize_t numb_of_read_bytes = 0;

//-----
if (argc != 2)
        {
        printf ("Usage: %s [FILE_NAME]\n", argv[0]);
        exit (EXIT_FAILURE);
        }
//-----


//-----
// maybe just open with O_CREAT flag?
if (mkfifo (SND_FIFO, S_IRUSR | S_IWUSR))
        if (errno != EEXIST)
                HANDLE_ERROR("snd_write mkfifo");

if ((snd_write_desc = open (SND_FIFO, O_WRONLY)) == -1)
        HANDLE_ERROR("snd_write fifo open");

if (fcntl (snd_write_desc, F_SETFL, O_NONBLOCK))
        HANDLE_ERROR("fcntl snd_write nonblock");

if (fcntl (snd_write_desc, F_SETPIPE_SZ, PIPE_BUF) <= 0)
        HANDLE_ERROR("fcntl snd_write size");
assert (PIPE_BUF == fcntl (snd_write_desc, F_GETPIPE_SZ));
//-----
if (write (snd_write_desc, &snd_buf, PIPE_BUF) < 0)
        {
        printf ("You can not have more than one sender!\n");
        if (close (snd_write_desc))
                HANDLE_ERROR("snd_write_desc close");
        printf ("Exiting.\n");
        exit (EXIT_FAILURE);
        }
//-----


//-----
if (mkfifo (RCV_FIFO, S_IRUSR | S_IWUSR))
        if (errno != EEXIST)
                HANDLE_ERROR("rcv_read mkfifo");

if ((rcv_read_desc = open (RCV_FIFO, O_RDONLY)) == -1)
        HANDLE_ERROR("rcv fifo open");
//-----


//-----
file_name = argv[1];

if ((file_read_desc = open (file_name, O_RDONLY)) == -1)
        HANDLE_ERROR("file open");
//-----
if (mkfifo (DATA_FIFO, S_IRUSR | S_IWUSR))
        if (errno != EEXIST)
                HANDLE_ERROR("data mkfifo");

if ((fifo_write_desc = open (DATA_FIFO, O_WRONLY)) == -1)
        HANDLE_ERROR("data fifo open");
//-----
while ((numb_of_read_bytes = read (file_read_desc, &buf, BLOCK_SIZE)) > 0)
        {
        if (write (fifo_write_desc, &buf, numb_of_read_bytes) < 0)
                HANDLE_ERROR("write");
        }

if (numb_of_read_bytes < 0)
        HANDLE_ERROR("read");
//-----

if (close (fifo_write_desc))
        HANDLE_ERROR("data fifo close");

if (close (file_read_desc))
        HANDLE_ERROR("file close");

if (close (snd_write_desc))
        HANDLE_ERROR("snd_write_desc close");

//-----

if (read (rcv_read_desc, &snd_buf, PIPE_BUF) < 0)
        HANDLE_ERROR("rcv_read_desc read");

if (close (rcv_read_desc))
        HANDLE_ERROR("rcv_read_desc close");

exit (EXIT_SUCCESS);
}
