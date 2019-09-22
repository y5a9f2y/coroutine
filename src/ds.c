#include <stdlib.h>
#include <string.h>

#include "ds.h"

_co_list_t *_co_list_create() {
    _co_list_t *alloc = (_co_list_t *)malloc(sizeof(_co_list_t));
    if(!alloc) {
        return NULL;
    }
    alloc->next = alloc;
    alloc->prev = alloc;
    return alloc;
}

void _co_list_init(_co_list_t *l) {
    l->next = l;
    l->prev = l;
}

void _co_list_destroy(_co_list_t *l) {
    if(l) {
        free(l);
    }
}

void _co_list_insert(_co_list_t *l, _co_list_t *node) {
    node->next = l->next;
    node->prev = l;
    l->next->prev = node;
    l->next = node;
}

_co_list_t *_co_list_delete(_co_list_t *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = node;
    node->prev = node;
    return node;
}

int _co_list_empty(_co_list_t *l) {
    if(l->next == l) {
        return 1;
    } else {
        return 0;
    }
}
