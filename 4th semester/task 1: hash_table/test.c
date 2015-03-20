#include "hash_table.h"
#include "error.h"
#include <stdio.h>
#include <malloc.h>


size_t cool_hash_func (const hash_key_t* hash_key, size_t hash_key_size, size_t hash_table_size)
        {
        return (*(int*)hash_key) % hash_table_size;
        }

int main ()
{
int ret_val = 0;
hash_table_ptr test = NULL;
// void* hash_elem = NULL;

// char key[] = "this is key";

printf ("\n");
// int Hash_table_construct (hash_table_ptr* hash_table, size_t hash_table_size, hash_func hashing_func);
// int Hash_table_destruct  (hash_table_ptr* hash_table);
ret_val = Hash_table_construct (&test, 20, cool_hash_func);
printf ("construct ret_val = %d\n", ret_val);
my_perror ("after construct");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_destruct (&test);
printf ("destruct ret_val = %d\n", ret_val);
my_perror ("after destruct");
printf ("\n");
printf ("\n");

//-----
/* make func not-static to test
my_errno = 0;
hash_elem = _create_hash_elem (key, sizeof (key), key, sizeof (key));
my_perror ("after _create_hash_elem ");
printf ("\n");

my_errno = 0;
ret_val = _delete_hash_elem_from_mem (&hash_elem);
printf ("_delete_hash_elem_from_mem ret_val = %d\n", ret_val);
my_perror ("after _delete_hash_elem_from_mem");
printf ("\n");
printf ("\n");
*/

/*
my_errno = 1;
my_perror ("test");
*/
// printf ("%d", errno_error_definit[4]);

return 0;
}
