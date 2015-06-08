#include "reachability_of_solver.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include <string.h>
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


extern pthread_mutex_t mutex;

//------------------------------------------------------------------------------
// Prover
//------------------------------------------------------------------------------
void* reachability_of_solver_prover (void* reach_of_solv_prov_arg_ptr)
{
char udp_prove_msg[] = UDP_PROVE_SOLVER_REACHABILITY_MSG;
int numb_of_sent_bytes = 0;

struct sockaddr_in tasker_addr = ((struct reach_of_solv_prov_arg*)reach_of_solv_prov_arg_ptr)->tasker_addr;
int                udp_sk      = ((struct reach_of_solv_prov_arg*)reach_of_solv_prov_arg_ptr)->udp_sk;
int                my_numb     = ((struct reach_of_solv_prov_arg*)reach_of_solv_prov_arg_ptr)->my_numb;

struct sockaddr_in addr = {0};

addr.sin_family      = AF_INET;
addr.sin_port        = START_PORT + my_numb;
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

        sleep (SOLVER_PROVE_SLEEP_PERIOD);
        }

printf ("Should never get here!\n");
pthread_exit (NULL);
}
//------------------------------------------------------------------------------
// Tester
//------------------------------------------------------------------------------
void* reachability_of_solver_tester (void* reach_of_solv_test_arg_ptr)
{
int      udp_sk = 0;
//d uint32_t ip = 0;

int  numb_of_recv_bytes = 0;
char udp_buf[sizeof (UDP_PROVE_SOLVER_REACHABILITY_MSG) + 1] = "\0";

struct sockaddr_in tasker_addr = {0}; //d addr = {0};
socklen_t          tasker_addr_size = 0;

int i = 0;
int is_first_recv = 0;

int is_result_sent = 0;
int mutex_ret_val  = 0;

int numb_of_connected_solvers =
        ((struct reach_of_solv_test_arg*)reach_of_solv_test_arg_ptr)->numb_of_connected_solvers;
struct solver_tester_info* arr_of_solver_testers =
        ((struct reach_of_solv_test_arg*)reach_of_solv_test_arg_ptr)->arr_of_solver_testers;
int* arr_of_if_res_received =
        ((struct reach_of_solv_test_arg*)reach_of_solv_test_arg_ptr)->arr_of_if_res_received;

while (1)
        {
        sleep (SOLVER_TEST_SLEEP_PERIOD);

        for (i = 0; i < numb_of_connected_solvers; i++)
                {
                //printf ("testing solver reachability: recvfrom...\n");

                udp_sk = arr_of_solver_testers[i].udp_sk;
                //d ip     = arr_of_solver_testers[i].ip;
                is_first_recv = 1;

                do
                        {
                        tasker_addr_size = sizeof (tasker_addr);

                        numb_of_recv_bytes = recvfrom (udp_sk, udp_buf, sizeof (UDP_PROVE_SOLVER_REACHABILITY_MSG), 0,
                                                        (struct sockaddr*)(&tasker_addr), &tasker_addr_size);

                        // some error
                        if ((numb_of_recv_bytes == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
                                HANDLE_ERROR ("recvfrom testing tasker reachability");
                        // nothing to read
                        else if ((numb_of_recv_bytes == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
                                {
                                if (is_first_recv == 1)
                                        {
                                        //----------
                                        if ((mutex_ret_val = pthread_mutex_lock (&mutex)) != 0)
                                                {
                                                printf ("Error in pthread_mutex_lock while checking result, ret_val = %d\n",
                                                                                                                mutex_ret_val);
                                                exit (EXIT_FAILURE);
                                                }

                                        is_result_sent = arr_of_if_res_received[i];

                                        if ((mutex_ret_val = pthread_mutex_unlock (&mutex)) != 0)
                                                {
                                                printf ("Error in pthread_mutex_unlock while checking result, ret_val = %d\n",
                                                                                                                mutex_ret_val);
                                                exit (EXIT_FAILURE);
                                                }
                                        //----------
                                        // Network is down
                                        if (is_result_sent == 0)
                                                {
                                                printf ("\nLost connection with solver on port %d (proven by tester 1)!\n",
                                                                                                                START_PORT + i);
                                                printf ("Terminating the program...\n");
                                                exit   (EXIT_FAILURE);
                                                }
                                        // solver ended calculation, exited the program and
                                        // because of that stopped sending reachability messages
                                        else
                                                break;
                                        }

                                        /*
                                        addr.sin_family      = AF_INET;
                                        addr.sin_port        = htons (PORT);
                                        addr.sin_addr.s_addr = ip;
                                        memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

                                        if ((sendto (udp_sk, "test", strlen ("test"), 0,
                                                                          (struct sockaddr*)(&addr), sizeof (addr))) == -1)
                                                {
                                                printf ("break!\n");
                                                perror ("sendto");
                                                break;
                                                }

                                        printf ("\nLost connection with solver on port %d (proven by tester 1)!\n", START_PORT + i);
                                        printf ("Terminating the program...\n");
                                        exit   (EXIT_FAILURE);
                                        }
                                                */
                                else
                                        // we just have read all the data
                                        break;
                                }
                        // for TCP: solver closed the connection, for UDP received NULL sized msg(?)
                        else if (numb_of_recv_bytes == 0)
                                {
                                printf ("NULL!\n");
                                assert (!"Should never get here!");
                                break;
                                }

                        //udp_buf[numb_of_recv_bytes] = '\0';
                        //printf ("packet contains: \"%s\" from solver on port %d\n\n", udp_buf, START_PORT + i);

                        is_first_recv = 0;
                        }
                while (numb_of_recv_bytes > 0);
                }
        }

printf ("Should never get here!\n");
pthread_exit (NULL);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Undef error handling macroses
//------------------------------------------------------------------------------
#undef IS_DEBUG
#undef HANDLE_ERROR
#undef HANDLE_ERROR_wL
//------------------------------------------------------------------------------

