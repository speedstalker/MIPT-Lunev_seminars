#ifndef __PART_OF_SIMP_INT_INCLUDED
#define __PART_OF_SIMP_INT_INCLUDED


//----------
#define f(x) (x)
#define FINENESS (0.000001)
#define FROM     (-1000)
#define TO       ( 1000)
//----------
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
//----------
void* simpsons_rule_integral (void* thread_numb);
//----------


#endif // __PART_OF_SIMP_INT_INCLUDED

