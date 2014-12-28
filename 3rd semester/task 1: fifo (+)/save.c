// sender.c
//-----
if (mkfifo (END_SYNC_FIFO, S_IRUSR | S_IWUSR))
        if (errno != EEXIST)
                HANDLE_ERROR("end_sync_read mkfifo");

if ((end_sync_read_desc = open (END_SYNC_FIFO, O_RDONLY)) == -1)
        HANDLE_ERROR("end_sync_read fifo open");

if (close (end_sync_read_desc))
        HANDLE_ERROR("end_sync_read fifo close");
//-----
// after this all had been read/written by sender/receiver


// receiver.c
//-----
if (mkfifo (END_SYNC_FIFO, S_IRUSR | S_IWUSR))
        if (errno != EEXIST)
                HANDLE_ERROR("end_sync_write mkfifo");

if ((end_sync_write_desc = open (END_SYNC_FIFO, O_WRONLY)) == -1)
        HANDLE_ERROR("end_sync_write fifo open");

if (close (end_sync_write_desc))
        HANDLE_ERROR("end_sync_write fifo close");
//-----
// after this all had been read/written by sender/receiver
