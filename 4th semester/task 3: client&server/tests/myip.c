#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include <netdb.h>
extern int h_errno;

#define h_addr h_addr_list[0] /* for backward compatibility */

#include <arpa/inet.h>

//------------------------------------------------------------------------------
// Error handling macroses
//------------------------------------------------------------------------------
#define IS_DEBUG 1

#if (IS_DEBUG == 1)
//{
   #define HANDLE_ERROR HANDLE_ERROR_wL
   #define HANDLE_ERROR_wL(msg)                                                    \
                   do                                                              \
                   {                                                               \
                   char err_msg[256] = {0};                                        \
                                                                                   \
                   snprintf (err_msg, 255, "%d. " msg "%c", __LINE__, '\0');       \
                   perror (err_msg);                                               \
                   exit   (EXIT_FAILURE);                                          \
                   }                                                               \
                   while (0)
//}
#else
//{
   #define HANDLE_ERROR_wL HANDLE_ERROR
   #define HANDLE_ERROR(msg) \
                  do { perror(msg); exit(EXIT_FAILURE); } while (0)
//}
#endif
//------------------------------------------------------------------------------

int main ()
{
char name[HOST_NAME_MAX + 1];
memset (name, HOST_NAME_MAX + 1, sizeof (char));

if (gethostname (name, HOST_NAME_MAX))
        HANDLE_ERROR ("gethostname");
printf ("my name is: %s\n\n", name);

struct hostent* ip_holder = NULL;
ip_holder = gethostbyname (name);
printf ("my host is: %s\n\n", ip_holder->h_name);

printf ("my names are:\n");
char** iter = NULL;
for (iter = ip_holder->h_aliases; *iter != NULL; iter += 1)
        printf ("%s\n", *iter);
printf ("\n\n");

printf ("my ips are:\n");
iter = NULL;
for (iter = ip_holder->h_addr_list; *iter != NULL; iter += 1)
        printf ("%s\n", inet_ntop(AF_INET, *iter, name, HOST_NAME_MAX));
printf ("\n\n");

return 0;
}

