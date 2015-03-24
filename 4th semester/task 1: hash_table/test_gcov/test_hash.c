#include "hash_table.h"
#include "error.h"
#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>


/*
#include "list.h"

typedef struct Hash_elem hash_elem_t;
struct Hash_elem
        {
        list_node_t   list_node;

        hash_key_t*   key;
        size_t        key_size;
        hash_entry_t* entry;
        size_t        entry_size;
        };

typedef struct Hash_table
        {
        size_t        size;
        hash_func_t   func;
        list_node_t** idx_arr;
        } hash_table_t;

typedef struct Iterator
        {
        const hash_table_t* hash_table;
        list_node_t*        list_node;
        } iterator_t;
*/






size_t cool_hash_func (const hash_key_t* hash_key, size_t hash_key_size, size_t hash_table_size)
        {
        return (*(int*)hash_key) % hash_table_size;
        }

int main ()
{
size_t i = 0;
int ret_val = 0;
hash_table_ptr test = NULL;
iterator_ptr test_iter = NULL, test_iter2 = NULL, test_find_iter = NULL;

char*  key_buffer   = NULL;
char*  entry_buffer = NULL;

char key1[] = "1 this is key1";
char key2[] = "1 this is key2";
char key3[] = "3 this is key 3";
char key4[] = "4 oh, this is key 4";

char entry4[] = "oh, this is entry 4";

printf ("\n");
// int Hash_table_construct (hash_table_ptr* hash_table, size_t hash_table_size, hash_func hashing_func);
// int Hash_table_destruct  (hash_table_ptr* hash_table);
my_errno = 0;
ret_val = Hash_table_construct (&test, 20, cool_hash_func);
printf ("construct ret_val = %d\n", ret_val);
my_perror ("after construct\n\t");
printf ("\n");


// int Hash_table_add_elem    (hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size,
//                                                        const hash_entry_t* entry_to_add, size_t entry_size);
// int Hash_table_remove_elem (hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size);
my_errno = 0;
ret_val = Hash_table_add_elem (test, key1, sizeof (key1), key1, sizeof (key1));
printf ("add1_1 ret_val = %d\n", ret_val);
my_perror ("after add1_1\n\t");
printf ("\n");

my_errno = 0;
test_iter = get_iterator (test);
printf ("get_iterator test_iter = %p\n", test_iter);
my_perror ("after get_iterator\n\t");
printf ("\n");

my_errno = 0;
test_find_iter = find_elem (test, key1, sizeof (key1));
printf ("find_elem_1 test_find_iter = %p\n", test_find_iter);
my_perror ("after find_elem_1\n\t");
printf ("\n");

printf ("before memcmp\n");
assert (! memcmp (test_iter, test_find_iter, sizeof (16)));
printf ("after memcmp\n\n");

my_errno = 0;
ret_val = delete_iterator (&test_find_iter);
printf ("delete_iterator_1 ret_val = %d\n", ret_val);
my_perror ("after delete_iterator_1\n\t");
printf ("\n");


my_errno = 0;
test_iter2 = dup_iterator (test_iter);
printf ("get_iterator_2 test_iter2 = %p\n", test_iter2);
my_perror ("after get_iterator_2\n\t");
printf ("\n");

my_errno = 0;
ret_val = delete_iterator (&test_iter);
printf ("delete_iterator_2 ret_val = %d\n", ret_val);
my_perror ("after delete_iterator_2\n\t");
printf ("\n");

test_iter = test_iter2;
my_errno = 0;
ret_val = Hash_table_delete_elem (&test_iter2);
printf ("Hash_table_delete_elem ret_val = %d\n", ret_val);
my_perror ("after Hash_table_delete_elem\n\t");
printf ("\n");
// printf ("\t\tafter delete test_iter = %p\n", test_iter);
// printf ("\t\tafter delete test_iter2 = %p\n", test_iter2);

my_errno = 0;
test_find_iter = find_elem (test, key1, sizeof (key1));
printf ("find_elem_2 test_find_iter = %p\n", test_find_iter);
if (my_errno == entry_not_found)
        printf ("after find_elem_2\n\t: no error has occurred\n");
else
        my_perror ("after find_elem_2\n\t");
printf ("\n");

/*
my_errno = 0;
ret_val = Hash_table_remove_elem (test, key1, sizeof (key1));
printf ("remove_1 ret_val = %d\n", ret_val);
my_perror ("after remove_1");
printf ("\n");
*/

my_errno = 0;
ret_val = Hash_table_add_elem (test, key1, sizeof (key1), key1, sizeof (key1));
printf ("add1_2 ret_val = %d\n", ret_val);
my_perror ("after add1_2\n\t");
printf ("\n");

test_iter = get_iterator (test);
printf ("get_iterator_2 test_iter = %p\n", test_iter);
my_perror ("after get_iterator_2\n\t");
printf ("\n");

test_iter = move_next (test_iter);
printf ("move_next_1 test_iter = %p\n", test_iter);
my_perror ("after move_next_1\n\t");
printf ("\n");

test_iter = move_prev (test_iter);
printf ("move_prev_1 test_iter = %p\n", test_iter);
my_perror ("after move_prev_1\n\t");
printf ("\n");

my_errno = 0;
test_iter2 = find_elem (test, key1, sizeof (key1));
printf ("find_elem_3 test_iter2 = %p\n", test_iter2);
my_perror ("after find_elem_3\n\t");
printf ("\n");

test_iter2 = move_next (test_iter2);
printf ("move_next_2 test_iter2 = %p\n", test_iter2);
my_perror ("after move_next_2\n\t");
printf ("\n");

test_iter2 = move_prev (test_iter2);
printf ("move_prev_2 test_iter2 = %p\n", test_iter2);
my_perror ("after move_prev_2\n\t");
printf ("\n");

printf ("before memcmp\n");
assert (! memcmp (test_iter, test_iter2, sizeof (16)));
printf ("after memcmp\n\n");

my_errno = 0;
ret_val = delete_iterator (&test_iter);
printf ("delete_iterator_3 ret_val = %d\n", ret_val);
my_perror ("after delete_iterator_3\n\t");
printf ("\n");

my_errno = 0;
ret_val = delete_iterator (&test_iter2);
printf ("delete_iterator_4 ret_val = %d\n", ret_val);
my_perror ("after delete_iterator_4\n\t");
printf ("\n");


my_errno = 0;
ret_val = Hash_table_add_elem (test, key2, sizeof (key2), key2, sizeof (key2));
printf ("add2 ret_val = %d\n", ret_val);
my_perror ("after add2\n\t");
printf ("\n");

my_errno = 0;
ret_val = Hash_table_add_elem (test, key3, sizeof (key3), key3, sizeof (key3));
printf ("add3 ret_val = %d\n", ret_val);
my_perror ("after add3\n\t");
printf ("\n");

test_iter = get_iterator (test);
printf ("get_iterator_3 test_iter = %p\n", test_iter);
my_perror ("after get_iterator_3\n\t");
printf ("\n");

for (i = 0; i < 2; i++)
        test_iter = move_next (test_iter);
printf ("move_next_4 test_iter = %p\n", test_iter);
my_perror ("after move_next_4\n\t");
printf ("\n");

my_errno = 0;
ret_val = is_end (test_iter);
printf ("is_end_1 ret_val = %d\n", ret_val);
my_perror ("after is_end_1\n\t");
printf ("\n");

// printf ("%s\n",(char*)((container_of (test_iter->list_node, hash_elem_t, list_node))->entry));

assert (ret_val == 1);

// TODO:
my_errno = 0;
key_buffer = get_key (test_iter);
printf ("get_key_1 key_buffer = %p; %s\n", key_buffer, key_buffer);
my_perror ("after get_key_1\n\t");
printf ("\n");

my_errno = 0;
entry_buffer = get_entry (test_iter);
printf ("get_entry_1 entry_buffer = %p; %s\n", entry_buffer, entry_buffer);
my_perror ("after get_entry_1\n\t");
printf ("\n");

assert (! (memcmp (key_buffer, entry_buffer, sizeof (key3))));

free (key_buffer  ); key_buffer   = NULL;
free (entry_buffer); entry_buffer = NULL;

my_errno = 0;
ret_val = Hash_table_add_elem (test, key4, sizeof (key4), entry4, sizeof (entry4));
printf ("add4 ret_val = %d\n", ret_val);
my_perror ("after add4\n\t");
printf ("\n");


my_errno = 0;
ret_val = is_end (test_iter);
printf ("is_end_2 ret_val = %d\n", ret_val);
my_perror ("after is_end_2\n\t");
printf ("\n");

assert (ret_val == 0);

my_errno = 0;
ret_val = delete_iterator (&test_iter);
printf ("delete_iterator_7 ret_val = %d\n", ret_val);
my_perror ("after delete_iterator_7\n\t");
printf ("\n");


/*
my_errno = 0;
ret_val = Hash_table_remove_elem (test, key4, sizeof (key4));
printf ("remove_4 ret_val = %d\n", ret_val);
my_perror ("after remove_4");
printf ("\n");
*/

my_errno = 0;
ret_val = Hash_table_add_elem (test, key4, sizeof (key4), entry4, sizeof (entry4));
printf ("add4 ret_val = %d\n", ret_val);
my_perror ("after add4\n\t");
printf ("\n");

my_errno = 0;
test_iter2 = find_elem (test, key4, sizeof (key4));
printf ("find_elem_4 test_iter2 = %p\n", test_iter2);
my_perror ("after find_elem_4\n\t");
printf ("\n");

for (i = 0; i < 4; i++)
        test_iter2 = move_next (test_iter2);
printf ("move_next_3 test_iter2 = %p\n", test_iter2);
my_perror ("after move_next_3\n\t");
printf ("\n");

my_errno = 0;
test_iter = find_elem (test, key3, sizeof (key3));
printf ("find_elem_5 test_iter = %p\n", test_iter);
my_perror ("after find_elem_5\n\t");
printf ("\n");

for (i = 0; i < 3; i++)
        test_iter = move_prev (test_iter);
printf ("move_prev_3 test_iter = %p\n", test_iter);
my_perror ("after move_prev_3\n\t");
printf ("\n");

printf ("before memcmp\n");
assert (! memcmp (test_iter, test_iter2, sizeof (16)));
printf ("after memcmp\n\n");

my_errno = 0;
ret_val = delete_iterator (&test_iter);
printf ("delete_iterator_5 ret_val = %d\n", ret_val);
my_perror ("after delete_iterator_5\n\t");
printf ("\n");

my_errno = 0;
ret_val = delete_iterator (&test_iter2);
printf ("delete_iterator_6 ret_val = %d\n", ret_val);
my_perror ("after delete_iterator_6\n\t");
printf ("\n");


my_errno = 0;
ret_val = Hash_table_destruct (&test);
printf ("destruct ret_val = %d\n", ret_val);
my_perror ("after destruct\n\t");
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
