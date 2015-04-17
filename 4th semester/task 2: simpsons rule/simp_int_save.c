#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>


/**
  * container_of - cast a member of a structure out to the containing structure
  * @ptr:        the pointer to the member.
  * @type:       the type of the container struct this is embedded in.
  * @member:     the name of the member within the struct.
  *
  */
#define container_of(ptr, type, member)                                 \
                ({                                                      \
                const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
                (type *)( (char *)__mptr - offsetof(type,member) );     \
                })


#define HANDLE_ERROR(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)

// #define f(x) (sqrt(abs(1 - 5*(pow(sin(x), 2)))))
#define f(x) ((x))
// assign this defines to corresponding vars later in code, because
// in this way program easily adopts to getting this params from command line
#define FINENESS (0.0000001)
#define FROM     (-2000)
#define TO       ( 2000)


typedef struct general_data
        {
        long numb_of_threads;
        double fineness;

        double from;
        double to;

        // I know about flexible sized arrays, but I'm aware of future problems
        // with using this structure with 'container_of' macro
        double* arr_of_thread_numbs;
        } general_data_t;

static inline double simpsons_rule_formula  (double left_end, double right_end);
void*                simpsons_rule_integral (void* thread_numb);


int main (int argc, char* argv[])
{
char *str = NULL, *endptr = NULL;

size_t i = 0;
int ret_val = 0;
long numb_of_threads = 0;

general_data_t* general_data = NULL;
double          result = 0;

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
        {
        perror ("strtol");
        exit (EXIT_FAILURE);
        }

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

if (!(general_data = (general_data_t*)calloc (1, sizeof (general_data_t) +
                                              sizeof (*(general_data->arr_of_thread_numbs)) * numb_of_threads)))
        HANDLE_ERROR("calloc - creating structure");

printf ("general data = %p\n", general_data);

// assign this defines to corresponding vars here, because
// in this way program easily adopts to getting this params from command line
general_data->numb_of_threads = numb_of_threads;
general_data->fineness        = FINENESS;
general_data->from            = FROM;
general_data->to              = TO;

general_data->arr_of_thread_numbs = (typeof (general_data->arr_of_thread_numbs))(general_data + 1);
printf ("arr_of_thread_numbs = %p\n", &(general_data->arr_of_thread_numbs));

/*
printf ("%x\n", sizeof (general_data_t));

printf ("%p\n", general_data);
printf ("%p\n", general_data->arr_of_thread_numbs);
printf ("%p\n", &(general_data->arr_of_thread_numbs[0]));
printf ("%p\n", &(general_data->arr_of_thread_numbs[1]));

printf ("%p\n", &(general_data->arr_of_thread_numbs[numb_of_threads - 1]));
printf ("%p\n", (char*)general_data + sizeof (general_data_t) + sizeof (long) * numb_of_threads);
*/

for (i = 0; i < numb_of_threads; i++)
        general_data->arr_of_thread_numbs[i] = i;

//------------------------------------------------------------------------------
// All thread's routines
//------------------------------------------------------------------------------
pthread_t thread_ids[numb_of_threads];
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
        printf ("Error in pthread_attr_setdetachstate: ret_val = %d\n", ret_val);
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

printf ("result = %f\n", result);

free (general_data); general_data = NULL;

return 0;
}


void* simpsons_rule_integral (void* thread_numb)
{
double* my_numb_ptr = (double*)thread_numb;
double  my_numb     = *my_numb_ptr;

double i = 0;
double my_from = 0, my_to = 0, fineness = 0;
double my_result = 0;


general_data_t* general_data = (general_data_t*)(my_numb_ptr - (size_t)my_numb) - 1;

// find out my own piece of (FROM;TO) to calculate integral on it
my_from  = (general_data->to - general_data->from) * my_numb       / (general_data->numb_of_threads) + general_data->from;
my_to    = (general_data->to - general_data->from) * (my_numb + 1) / (general_data->numb_of_threads) + general_data->from;
fineness = general_data->fineness;

for (i = my_from; i < my_to; i += fineness)
        my_result += simpsons_rule_formula (i, i + fineness);

printf ("simpsons_formula for 1, 2 = %f\n", simpsons_rule_formula (10, 20));
printf ("I'm №%zd, my_general_data = %p, my_result = %f\n", (size_t)(my_numb), general_data, my_result);
*my_numb_ptr = my_result;

pthread_exit (NULL);
}

static inline double simpsons_rule_formula (double left_end, double right_end)
{
return ((right_end - left_end)/6 * (f(left_end) + 4*f((left_end + right_end)/2) + f(right_end)));
}

