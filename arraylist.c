#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

/* Note: entry_size * index converts the index of an entry to a byte offset
 * from the beginning of the table; */

/* initialize table, all values set to 0 */
uint8_t *create_table(int num_entries, int entry_size) {
    uint8_t *table;
    table = (uint8_t*)calloc(num_entries, entry_size);
    if (table == NULL) {
	perror("Generating Table");
	exit(EXIT_FAILURE);
	}
    return table;
}

uint8_t *realloc_table(uint8_t *table, int old_size, int new_size, int entry_size) {
    int i;
    if (table == NULL)
	return NULL;
    if (NULL == (realloc(table, new_size * entry_size)))
	return NULL;
    /* initializes the add values of the array to 0 */
    for (i = 0; i < new_size - old_size; i++)
	table[old_size + 1] = 0;
    return table;

}

/* bound checking is handled by the server, not the data structure itself */
uint8_t table_insert(uint8_t *table, void *entry, int entry_size, int index) {
    if (table == NULL || entry == NULL)
	return -1;
    if (NULL == (memcpy(table +(entry_size * index), entry, entry_size)))
	return -1;
    return 0;
}

/* 0s out the location of memory where the entry was */
uint8_t table_delete(uint8_t *table, int entry_size, int index) {
    int i;
    if (table == NULL)
	return -1;
    for (i = 0; i < entry_size; i++) {
	table[index*entry_size + i] = 0;
    }
    return 0;
}

/* returns a void pointer to the first byte of the entry */
void *table_fetch(uint8_t *table, int entry_size, int index) {
    void *entry = NULL;
    if (table == NULL)
	return NULL;
    entry = table + (entry_size * index);
    return entry;
}

/* TESTING. REMOVE IN FINAL BUILD  
int main(){
    uint8_t *table = create_table(10, 5);
    char *str = "Helo";
    char *other = NULL;
    if (table_insert(table, str, 5, 0) == 0) {
	other = (char *)table_fetch(table, 5, 0);
    }
    printf("%s\n", other);
    realloc_table(table, 10, 20, 5);
    table_insert(table, str, 5, 15);
    printf("%s\n", table + 15*5);
    return 0;
}
*/
