#ifndef ARRAYLIST_H
#define ARRAYLIST_H

uint8_t *create_table(int num_entries, int entry_size);

uint8_t *realloc_table(uint8_t *table, int old_size, int new_size, int entry_size);

uint8_t table_insert(uint8_t *table, void *entry, int entry_size, int index);

uint8_t table_delete(uint8_t *table, int entry_size, int index);

void *table_fetch(uint8_t *table, int entry_size, int index);

#endif
