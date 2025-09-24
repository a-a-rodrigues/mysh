#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arraylist.h"

#define INITIAL_CAPACITY 10

char* strdup(const char* str) {
    size_t len = strlen(str) + 1;  // +1 for the null terminator
    char* new_str = (char*)malloc(len);
    if (new_str == NULL) {
        return NULL; // Memory allocation failed
    }
    strcpy(new_str, str);
    return new_str;
}

arraylist_t* al_init() {
    arraylist_t* list = (arraylist_t*)malloc(sizeof(arraylist_t));
    if (list == NULL) {
        return NULL; // Allocation failed
    }

    list->data = (char**)malloc(INITIAL_CAPACITY * sizeof(char*));
    if (list->data == NULL) {
        free(list);
        return NULL; // Allocation failed
    }

    list->size = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

void al_destroy(arraylist_t* list) {
    for (size_t i = 0; i < list->size; ++i) {
        free(list->data[i]); // Free individual strings
    }
    free(list->data);
    free(list);
}

void al_append(arraylist_t* list, const char* value) {
    if (list->size >= list->capacity) {
        // Resize the array if needed
        list->capacity *= 2;
        list->data = realloc(list->data, list->capacity * sizeof(char*));
        if (list->data == NULL) {
            // Memory reallocation failed
            // Handle error appropriately
            return;
        }
    }
    list->data[list->size] = strdup(value); // Duplicate the string
    if (list->data[list->size] == NULL) {
        // Memory allocation for string failed
        // Handle error appropriately
        return;
    }
    list->size++;
}

const char* al_get(arraylist_t* list, int index) {
    if ((index < 0) || (index >= list->size)) {
        // Index out of bounds
        // Handle error appropriately
        return NULL;
    }
    return list->data[index];
}

// Usage: al_sever is meant to split an array list into two parts at the index specified
// It cuts down the old arraylist and returns the "severed" portion

arraylist_t* al_sever(arraylist_t *list, int index) {
    if ((index >= list->size - 1) || (index <= 0)) {
        return NULL;
    }
    arraylist_t *slice = al_init();

    for (int i = index + 1; i < list->size; i++) {
        al_append(slice, list->data[i]);
        free(list->data[i]);
    }
    
    list->size = (list->size - index - 1);
    
    return slice;
}

// This function will take a string argument and return its position in the arraylist, else returns -1
int al_contains(arraylist_t *list, char *token) {

    for (int i = 0; i < list->size; i++) {
        if (strcmp(token, al_get(list, i)) == 0) return i;
    }

    return -1;
}

void al_remove(arraylist_t* list, int index) {
    if (index < 0 || index >= list->size) {
        // Invalid index, do nothing
        return;
    }

    // Free the string at the specified index
    free(list->data[index]);

    // Shift the remainder of the list to the left
    for (int i = index; i < list->size - 1; i++) {
        list->data[i] = list->data[i + 1];
    }

    // Decrement the size of the list
    list->size--;


}