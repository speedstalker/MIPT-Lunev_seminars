#include "hash_table.h"
#include <stdio.h>
#include <malloc.h>

int cool_hash_func (const hash_key_t* key)
        {
        return 0;
        }

int main ()
{
int ret_val = 0;
hash_table_ptr test = NULL;

// int  Hash_table_construct (hash_table_ptr* hash_table, size_t hash_table_size, int (*hash_func)(const hash_key_t*));
// void Hash_table_destruct  (hash_table_ptr* hash_table);
ret_val = Hash_table_construct (&test, 20, cool_hash_func);
printf ("construct ret_val = %d\n", ret_val);
my_perror ("after construct");

my_errno = 0;
Hash_table_destruct (&test);
my_perror ("after destruct");


/*
my_errno = 1;
my_perror ("test");
*/

return 0;
}
