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

char key1[] = "this is key1";
char key2[] = "this is key2";
char key3[] = "this is key 3";
char key4[] = "oh, this is key 4";

char entry4[] = "oh, this is entry 4";

printf ("\n");
// int Hash_table_construct (hash_table_ptr* hash_table, size_t hash_table_size, hash_func hashing_func);
// int Hash_table_destruct  (hash_table_ptr* hash_table);
my_errno = 0;
ret_val = Hash_table_construct (&test, 20, cool_hash_func);
printf ("construct ret_val = %d\n", ret_val);
my_perror ("after construct");
printf ("\n");


// int Hash_table_add_elem    (hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size,
//                                                        const hash_entry_t* entry_to_add, size_t entry_size);
// int Hash_table_remove_elem (hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size);
my_errno = 0;
ret_val = Hash_table_add_elem (test, key1, sizeof (key1), key1, sizeof (key1));
printf ("add1_1 ret_val = %d\n", ret_val);
my_perror ("after add1_1");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_remove_elem (test, key1, sizeof (key1));
printf ("remove_1 ret_val = %d\n", ret_val);
my_perror ("after remove_1");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_add_elem (test, key1, sizeof (key1), key1, sizeof (key1));
printf ("add1_2 ret_val = %d\n", ret_val);
my_perror ("after add1_2");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_add_elem (test, key2, sizeof (key2), key2, sizeof (key2));
printf ("add2 ret_val = %d\n", ret_val);
my_perror ("after add2");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_add_elem (test, key3, sizeof (key3), key3, sizeof (key3));
printf ("add3 ret_val = %d\n", ret_val);
my_perror ("after add3");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_add_elem (test, key4, sizeof (key4), entry4, sizeof (entry4));
printf ("add4 ret_val = %d\n", ret_val);
my_perror ("after add4");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_remove_elem (test, key4, sizeof (key4));
printf ("remove_4 ret_val = %d\n", ret_val);
my_perror ("after remove_4");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_add_elem (test, key4, sizeof (key4), entry4, sizeof (entry4));
printf ("add4 ret_val = %d\n", ret_val);
my_perror ("after add4");
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
