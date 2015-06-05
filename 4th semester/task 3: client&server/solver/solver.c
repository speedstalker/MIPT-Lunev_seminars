#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
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

#define UDP_MSG                                 "Is there anybody out there?"
#define UDP_PROVE_TASKER_REACHABILITY_MSG       "Tasker is reachable!"
#define UDP_PROVE_SOLVER_REACHABILITY_MSG       "Solver is reachable!"

#define PROVE_TEST_PERIOD        5 // in sec
#define PROVE_SLEEP_PERIOD       1 // in sec
#define MAX_WAIT_FOR_TASK_TIME  10 // in sec

#define PORT            1234
#define UDP_PORT        1235
#define START_PORT      1236

#define f(x) (x)
#define FINENESS (0.000001)
#define FROM     (-1000)
#define TO       ( 1000)


typedef struct general_data
        {
        long        numb_of_threads;
        long double fineness;

        long double from;
        long double to;

        // I know about flexible sized arrays
        long double* arr_of_thread_numbs;       // I'm also using it as an each-thread-result-holder
                                                // that's why it needs to be long double
        } general_data_t;

static inline long double simpsons_rule_formula  (long double left_end, long double right_end);
void*                     simpsons_rule_integral (void* thread_numb);


void* reachability_of_tasker_tester (void* udp_sk_ptr);
void* reachability_of_solver_prover (void* reach_of_solv_prov_arg_ptr);

struct reach_of_solv_prov_arg
        {
        int udp_sk;
        uint16_t tcp_port;
        int my_numb;
        struct sockaddr_in tasker_addr;
        };


struct solver_task
        {
        int my_numb;
        int numb_of_connected_solvers;
        };

int main (int argc, char* argv[])
{
int i = 0;
int ret_val = 0;
//----------
char *str = NULL, *endptr = NULL;
long numb_of_threads = 0;
//----------
int udp_sk = 0;
char udp_buf[sizeof (UDP_MSG) + 1] = "\0";

int numb_of_recv_bytes = 0;
//----------
struct sockaddr_in addr = {0}, tasker_addr = {0};
socklen_t addr_size = 0, tasker_addr_size = 0;
//----------
int tcp_sk = 0;
struct solver_task my_task = {0};
//----------
general_data_t* general_data = NULL;
long double     result = 0;

int numb_of_sent_bytes = 0;

//------------------------------------------------------------------------------
// Getting number of threads
//------------------------------------------------------------------------------
if (argc != 2)
        {
        printf ("Usage: %s NUMB_OF_THREADS\n", argv[0]);
        exit (EXIT_FAILURE);
        }

str = argv[1];

errno = 0;
numb_of_threads = strtol (str, &endptr, 10);    // 10 - base of numerical system
if ((errno == ERANGE && (numb_of_threads == LONG_MAX || numb_of_threads == LONG_MIN))
                   || (errno != 0 && numb_of_threads == 0))
        HANDLE_ERROR ("strtol");

if ((endptr == str) || (*endptr != '\0'))
        {
        printf ("NUMB_OF_THREADS must contain only digits!\n");
        exit (EXIT_FAILURE);
        }

if (numb_of_threads <= 0)
        {
        printf ("NUMB_OF_THREADS must be strictly positive number!\n");
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------
// Create an UDP socket, bind it and receive the message
//------------------------------------------------------------------------------
if ((udp_sk = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
        HANDLE_ERROR ("udp_sk socket");
printf ("udp_sk has been created!\n");
//----------
addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = htonl (INADDR_ANY);
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

if (bind (udp_sk, (struct sockaddr*)(&addr), sizeof (addr)) == -1)
        HANDLE_ERROR ("udp_sk bind");
printf ("udp_sk has been binded!\n\n");
//----------
tasker_addr_size = sizeof (tasker_addr);
printf ("Waiting to recvfrom...\n");
if ((numb_of_recv_bytes = recvfrom (udp_sk, udp_buf, sizeof (UDP_MSG), 0,
                                    (struct sockaddr*)(&tasker_addr), &tasker_addr_size)) == -1)
        HANDLE_ERROR ("udp recvfrom");

printf ("got UDP packet from %s\n", inet_ntoa (tasker_addr.sin_addr));
printf ("packet is %d bytes long\n", numb_of_recv_bytes);
udp_buf[numb_of_recv_bytes] = '\0';
printf ("packet contains: \"%s\"\n\n", udp_buf);
//------------------------------------------------------------------------------
// Create a TCP socket and connect to tasker
//------------------------------------------------------------------------------
if ((tcp_sk = socket (PF_INET, SOCK_STREAM, 0)) == -1)
        HANDLE_ERROR ("tcp_sk socket");
printf ("tcp_sk has been created!\n");
//----------
addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = tasker_addr.sin_addr.s_addr;
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

printf ("trying to connect to %s on port %d\n", inet_ntoa(addr.sin_addr), PORT);
if (connect (tcp_sk, (struct sockaddr*)&addr, sizeof (addr)) == -1)
        HANDLE_ERROR ("tcp_sk connect");
printf ("tcp_sk has been connected to %s!\n\n", inet_ntoa (tasker_addr.sin_addr));
//------------------------------------------------------------------------------
// Receive the task from the tasker
//------------------------------------------------------------------------------
printf ("Waiting for the task...\n");
fd_set rfds; FD_ZERO (&rfds);
FD_SET (tcp_sk, &rfds);

struct timeval tv;
tv.tv_sec  = MAX_WAIT_FOR_TASK_TIME;
tv.tv_usec = 0;

if ((ret_val = select (tcp_sk + 1, &rfds, NULL, NULL, &tv)) == -1)
        HANDLE_ERROR ("select on task waiting");
else if (ret_val == 0)
        {
        printf ("\nLost connection with tasker (proven by select tester)!\n");
        printf ("Terminating the program...\n");
        exit   (EXIT_FAILURE);
        }

numb_of_recv_bytes = 0;
do
        {
        ret_val = 0;
        if ((ret_val = recv (tcp_sk,
                             &my_task + numb_of_recv_bytes,
                             sizeof (struct solver_task) - numb_of_recv_bytes, 0)) == -1)
                HANDLE_ERROR ("recv solver_task");
        if (ret_val == 0)
                {
                printf ("\nConnection to tasker lost!\n");
                printf ("terminating the program...\n");
                exit   (EXIT_FAILURE);
                }
        numb_of_recv_bytes += ret_val;
        }
while (numb_of_recv_bytes != sizeof (struct solver_task));

printf ("task has been received!\n");
printf ("My number is %d of total %d solver%s.\n\n", my_task.my_numb,
                                                     my_task.numb_of_connected_solvers,
                                                     (my_task.numb_of_connected_solvers == 1) ? "" : "s");
//------------------------------------------------------------------------------
// Make a thread to prove reachability of solver
//------------------------------------------------------------------------------
pthread_t reachability_of_solver_prover_thr_id = 0;
pthread_attr_t thread_attr;

if ((ret_val = pthread_attr_init (&thread_attr)))
        {
        printf ("Error in pthread_attr_init for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
if ((ret_val = pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_JOINABLE)))
        {
        printf ("Error in pthread_attr_setdetachstate for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

struct reach_of_solv_prov_arg reach_of_solv_prov_arg = {0};
reach_of_solv_prov_arg.udp_sk      = udp_sk;

addr_size = sizeof (addr);
if (getsockname (tcp_sk, (struct sockaddr*)&addr, &addr_size))
        HANDLE_ERROR ("getsockname tcp_sk");
reach_of_solv_prov_arg.tcp_port    = addr.sin_port;

reach_of_solv_prov_arg.my_numb     = my_task.my_numb;
reach_of_solv_prov_arg.tasker_addr = tasker_addr;

if ((ret_val = pthread_create (&reachability_of_solver_prover_thr_id,
                               &thread_attr,
                               reachability_of_solver_prover,
                               &reach_of_solv_prov_arg)))
        {
        printf ("Error in pthread_create for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_attr_destroy (&thread_attr)))
        {
        printf ("Error in pthread_attr_destroy for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------
// Make a thread to test reachability of tasker
//------------------------------------------------------------------------------
pthread_t reachability_of_tasker_tester_thr_id = 0;

if ((ret_val = pthread_attr_init (&thread_attr)))
        {
        printf ("Error in pthread_attr_init for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
if ((ret_val = pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_JOINABLE)))
        {
        printf ("Error in pthread_attr_setdetachstate for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_create (&reachability_of_tasker_tester_thr_id,
                               &thread_attr,
                               reachability_of_tasker_tester,
                               &udp_sk)))
        {
        printf ("Error in pthread_create for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_attr_destroy (&thread_attr)))
        {
        printf ("Error in pthread_attr_destroy for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------
// Piece of task_2: simp_int.c program, e.g. mathematical algorithm
//------------------------------------------------------------------------------
// actually allocs memory for general_data structure (treat it like a header for an array)
// and an array that have size: typeof(*arr_of_thread_numbs) * numb_of_threads
general_data = (general_data_t*)calloc (1, sizeof (general_data_t) +
                                              sizeof (*(general_data->arr_of_thread_numbs)) * numb_of_threads);
if (general_data == 0)
        HANDLE_ERROR("calloc - creating structure");

general_data->numb_of_threads = numb_of_threads;
general_data->fineness        = FINENESS;
if (my_task.my_numb == 0)
        general_data->from = FROM;
else
        general_data->from = (TO - FROM) / my_task.numb_of_connected_solvers * my_task.my_numb + FROM;

if (my_task.my_numb == (my_task.numb_of_connected_solvers - 1))
        general_data->to = TO;
else
        general_data->to = (TO - FROM) / my_task.numb_of_connected_solvers * (my_task.my_numb + 1) + FROM;

// (general_data + 1) points right behind the structure-header => to the first element of an array
// then we just find out what type do pointer to the array member have
general_data->arr_of_thread_numbs = (typeof (general_data->arr_of_thread_numbs))(general_data + 1);

for (i = 0; i < numb_of_threads; i++)
        general_data->arr_of_thread_numbs[i] = i;
//------------------------------------------------------------------------------
// All thread's routines
//------------------------------------------------------------------------------
pthread_t      thread_ids[numb_of_threads];

if ((ret_val = pthread_attr_init (&thread_attr)))
        {
        printf ("Error in pthread_attr_init: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
if ((ret_val = pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_JOINABLE)))
        {
        printf ("Error in pthread_attr_setdetachstate: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

for (i = 0; i < numb_of_threads; i++)
        if ((ret_val = pthread_create (&thread_ids[i],
                                       &thread_attr,
                                       simpsons_rule_integral,
                                       (general_data->arr_of_thread_numbs + i))))
                {
                printf ("Error in pthread_create: ret_val = %d\n", ret_val);
                exit (EXIT_FAILURE);
                }
printf ("Started calculating...\n");

if ((ret_val = pthread_attr_destroy (&thread_attr)))
        {
        printf ("Error in pthread_attr_destroy: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

for (i = 0; i < numb_of_threads; i++)
        if ((ret_val = pthread_join (thread_ids[i], NULL)))
                {
                printf ("Error in pthread_join: ret_val = %d\n", ret_val);
                exit (EXIT_FAILURE);
                }
printf ("finished calculating!\n");
//------------------------------------------------------------------------------
// if we came here - all threads have computed their parts
//------------------------------------------------------------------------------
for (i = 0; i < numb_of_threads; i++)
        result += general_data->arr_of_thread_numbs[i];

printf ("my result = %Lf\n\n", result);
//------------------------------------------------------------------------------
// Send my result to tasker
//------------------------------------------------------------------------------
printf ("Started sending the result...\n");
numb_of_sent_bytes = 0;
do
        {
        ret_val = 0;
        if ((ret_val = send (tcp_sk,
                             &result + numb_of_sent_bytes,
                             sizeof (result) - numb_of_sent_bytes, 0)) == -1)
                HANDLE_ERROR ("send result");
        numb_of_sent_bytes += ret_val;
        }
while (numb_of_sent_bytes != sizeof (result));
printf ("result has been sent to the tasker!\n\n");
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
free (general_data); general_data = NULL;

//----------
// Cancel tasker reachability tester
if ((ret_val = pthread_cancel (reachability_of_tasker_tester_thr_id)))
        {
        printf ("Error in pthread_cancel for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (reachability_of_tasker_tester_thr_id, NULL)))
        {
        printf ("Error in pthread_join for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//----------

close (tcp_sk);
close (udp_sk);

//----------
// Cancel solver reachability prover
ret_val = pthread_cancel (reachability_of_solver_prover_thr_id);
if ((ret_val != 0) && (ret_val != ESRCH))
        {
        printf ("Error in pthread_cancel for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (reachability_of_solver_prover_thr_id, NULL)))
        {
        printf ("Error in pthread_join for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------

return 0;
}


void* simpsons_rule_integral (void* thread_numb)
{
long double* my_numb_ptr = (long double*)thread_numb;
long double  my_numb     = *my_numb_ptr;

long double i = 0;      // not int because algorithm step with not int fineness
long double my_from = 0, my_to = 0, fineness = 0;
long double my_result = 0;


// (my_numb_ptr - (size_t)my_numb) - by doing this we get the pointer to the first array member
// and then we step back by general_data_t size, to get the pointer to general_data structure
general_data_t* general_data = (general_data_t*)(my_numb_ptr - (size_t)my_numb) - 1;

// find out my own piece of (FROM;TO) to calculate integral on it
if (my_numb == 0)
        my_from = general_data->from;
else
        my_from  = (general_data->to - general_data->from) * my_numb       / (general_data->numb_of_threads) + general_data->from;

if (my_numb == (general_data->numb_of_threads - 1))
        my_to = general_data->to;
else
        my_to    = (general_data->to - general_data->from) * (my_numb + 1) / (general_data->numb_of_threads) + general_data->from;

fineness = general_data->fineness;

for (i = my_from; i < my_to; i += fineness)
        my_result += simpsons_rule_formula (i, i + fineness);

*my_numb_ptr = my_result;

pthread_exit (NULL);
}

static inline long double simpsons_rule_formula (long double left_end, long double right_end)
{
return ((right_end - left_end)/6 * (f(left_end) + 4*f((left_end + right_end)/2) + f(right_end)));
}


void* reachability_of_tasker_tester (void* udp_sk_ptr)
{
int udp_sk = *(int*)udp_sk_ptr;
int numb_of_recv_bytes = 0;

char udp_buf[sizeof (UDP_MSG) + 1] = "\0";
struct sockaddr_in tasker_addr = {0};
socklen_t tasker_addr_size = 0;

int ret_val = 0;
int is_first_recv = 0;


if ((ret_val = fcntl (udp_sk, F_SETFL, O_NONBLOCK)) == -1)
        HANDLE_ERROR ("fcntl udp_sk O_NONBLOCK");

while (1)
        {
        // printf ("testing tasker reachability: recvfrom...\n");

        sleep (PROVE_TEST_PERIOD);
        is_first_recv = 1;

        do
                {
                tasker_addr_size = sizeof (tasker_addr);

                numb_of_recv_bytes = recvfrom (udp_sk, udp_buf, sizeof (UDP_PROVE_TASKER_REACHABILITY_MSG), 0,
                                                (struct sockaddr*)(&tasker_addr), &tasker_addr_size);

                // some error
                if ((numb_of_recv_bytes == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
                        HANDLE_ERROR ("recvfrom testing tasker reachability");
                // nothing to read
                else if ((numb_of_recv_bytes == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
                        {
                        if (is_first_recv == 1)
                                {
                                printf ("\nLost connection with tasker (proven by tester 1)!\n");
                                printf ("Terminating the program...\n");
                                exit   (EXIT_FAILURE);
                                }
                        else
                                break;
                        }
                // tasker closed the connection
                else if (numb_of_recv_bytes == 0)
                        {
                        printf ("\nLost connection with tasker (proven by tester 2)!\n");
                        printf ("Terminating the program...\n");
                        exit   (EXIT_FAILURE);
                        }

                // udp_buf[numb_of_recv_bytes] = '\0';
                // printf ("packet contains: \"%s\"\n\n", udp_buf);

                is_first_recv = 0;
                }
        while (numb_of_recv_bytes > 0);
        }

printf ("Should never get here!\n");
pthread_exit (NULL);
}

void* reachability_of_solver_prover (void* reach_of_solv_prov_arg_ptr)
{
char udp_prove_msg[] = UDP_PROVE_SOLVER_REACHABILITY_MSG;
int numb_of_sent_bytes = 0;

struct sockaddr_in tasker_addr = ((struct reach_of_solv_prov_arg*)reach_of_solv_prov_arg_ptr)->tasker_addr;
int                udp_sk      = ((struct reach_of_solv_prov_arg*)reach_of_solv_prov_arg_ptr)->udp_sk;
int                my_numb     = ((struct reach_of_solv_prov_arg*)reach_of_solv_prov_arg_ptr)->my_numb;

struct sockaddr_in addr = {0};

addr.sin_family      = AF_INET;
addr.sin_port        = START_PORT + my_numb; //tcp_port; //htons (UDP_PORT);
addr.sin_addr.s_addr = tasker_addr.sin_addr.s_addr;
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

printf ("start sending the UDP_PROVE_SOLVER_REACHABILITY_MSG on port %d...\n", START_PORT + my_numb);

while (1)
        {
        numb_of_sent_bytes = sendto (udp_sk, udp_prove_msg, strlen (udp_prove_msg), 0,
                                                  (struct sockaddr*)(&addr), sizeof (addr));
        // some error
        if ((numb_of_sent_bytes == -1) && (errno != EBADF))
                HANDLE_ERROR ("udp sendto");
        // sk has been closed before cancel() called
        else if ((numb_of_sent_bytes == -1) && (errno == EBADF))
                pthread_exit (NULL);

        //printf ("sent %d bytes: \"%s\"\n\n", numb_of_sent_bytes, udp_prove_msg);

        sleep (PROVE_SLEEP_PERIOD);
        }

printf ("Should never get here!\n");
pthread_exit (NULL);
}


