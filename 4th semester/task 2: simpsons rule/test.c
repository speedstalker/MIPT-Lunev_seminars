#include <stdio.h>
#include <stdlib.h>

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


typedef struct general_data
        {
        long numb_of_threads;
        double fineness;

        // I know about flexible sized arrays, but I'm aware of future problems
        // with using this structure with 'container_of' macro
        long* arr_of_thread_numbs;
        } general_data_t;

int main ()
{
general_data_t general_data;

typeof( "checking" ) str = "please";
printf("%s\n", str);

long long_sample = 5;
long* long_ptr_sample = &long_sample;
/* Works OK
typeof(&(general_data.arr_of_thread_numbs[0])) long_ptr = &long_ptr_sample;
printf("%ld\n", *long_ptr);
*/

// Works OK
typeof(general_data.arr_of_thread_numbs) long_ptr = long_ptr_sample;
printf("%ld\n", *long_ptr);

printf("%ld\n", general_data.arr_of_thread_numbs[1]);

return 0;
}
