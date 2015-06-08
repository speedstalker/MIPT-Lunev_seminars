#include "part_of_simp_int.h"

#include <stdlib.h>
#include <pthread.h>
#include <math.h>


static inline long double simpsons_rule_formula (long double left_end, long double right_end);

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

