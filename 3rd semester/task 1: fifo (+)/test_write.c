#include <unistd.h>
#include <errno.h>

int main ()
{
char buf[] = "hello";

if (write (1, &buf, -1) < 0)
        perror ("write");

return 0;
}
