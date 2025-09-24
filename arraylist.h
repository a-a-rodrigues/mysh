#ifndef _MYMALLOC_H
#define _MYMALLOC_H

typedef struct {
    char **data;
    int size;
    int capacity;
} arraylist_t;

char* strdup(const char* str);

arraylist_t* al_init();
void al_destroy(arraylist_t* list);

void al_append(arraylist_t *list, const char* value);
const char* al_get(arraylist_t *list, int index);
void al_remove(arraylist_t* list, int index);

arraylist_t* al_sever(arraylist_t *list, int index);

int al_contains(arraylist_t *list, char *token);

#endif