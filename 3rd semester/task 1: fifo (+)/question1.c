#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)

int is_files_the_same (int fd1, int fd2)
{
struct stat stat1, stat2;

if (fstat (fd1, &stat1))
        HANDLE_ERROR(fstat fd1);

if (fstat (fd2, &stat2))
        HANDLE_ERROR(fstat fd2);

return ((stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino));
}

int main ()
{
fd2 = open("/tmp/file", O_RDWR);
fd1 = open("/tmp/file", O_RDWR);

is_files_the_same (fd1, fd2);

return 0;
}

// my func can return FALSE if this program would be freezed by scheduler between
// two open's calls and in that time I manually delete "/tmp/file" and then create
// new "/tmp/file", so in_ino  will be different;
// or
// for example mount flash drive at "/tmp/file" and at least "st_dev" will be diff
