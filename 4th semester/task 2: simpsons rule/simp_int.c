#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>


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
                   perror (err_msg);                                              \
                   exit   (EXIT_FAILURE);                                         \
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


//#define f(x) (pow(x, 2))
#define f(x) (x)
// assign this defines to corresponding vars later in code, because
// in this way program easily adopts to getting this params from command line
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


int main (int argc, char* argv[])
{
char *str = NULL, *endptr = NULL;

size_t i = 0;
int ret_val = 0;
long numb_of_threads = 0;

general_data_t* general_data = NULL;
long double     result = 0;

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
if (errno != 0)
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
// actually allocs memory for general_data structure (treat it like a header for an array)
// and an array that have size: typeof(*arr_of_thread_numbs) * numb_of_threads
general_data = (general_data_t*)calloc (1, sizeof (general_data_t) +
                                              sizeof (*(general_data->arr_of_thread_numbs)) * numb_of_threads);
if (general_data == 0)
        HANDLE_ERROR("calloc - creating structure");

// assign this defines to corresponding vars here, because
// in this way program easily adopts to getting this params from command line
general_data->numb_of_threads = numb_of_threads;
general_data->fineness        = FINENESS;
general_data->from            = FROM;
general_data->to              = TO;

// (general_data + 1) points right behind the structure-header => to the first element of an array
// then we just find out what type do pointer to the array member have
general_data->arr_of_thread_numbs = (typeof (general_data->arr_of_thread_numbs))(general_data + 1);

for (i = 0; i < numb_of_threads; i++)
        general_data->arr_of_thread_numbs[i] = i;

//------------------------------------------------------------------------------
// All thread's routines
//------------------------------------------------------------------------------
pthread_t      thread_ids[numb_of_threads];
pthread_attr_t thread_attr;

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
//------------------------------------------------------------------------------
// if we came here - all threads have computed their parts
//------------------------------------------------------------------------------

for (i = 0; i < numb_of_threads; i++)
        result += general_data->arr_of_thread_numbs[i];

printf ("result = %Lf\n", result);

free (general_data); general_data = NULL;

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

