#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds.h"
#include "sched.h"

#define _CO_HEAP_INIT_CAP 1

static void _co_time_heap_adjust_up(_co_time_heap_t *, size_t);
static void _co_time_heap_adjust_down(_co_time_heap_t *, size_t);

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

_co_time_heap_t *_co_time_heap_create() {

    _co_time_heap_t *alloc;

    if(!(alloc = (_co_time_heap_t *)malloc(sizeof(_co_time_heap_t)))) {
        return NULL;
    }
    alloc->size = 0;
    alloc->cap = _CO_HEAP_INIT_CAP;
    if(!(alloc->heap = (_co_time_heap_node_t *)malloc(
        sizeof(_co_time_heap_node_t) * _CO_HEAP_INIT_CAP))) {
        free(alloc);
        errno = ENOMEM;
        return NULL;
    }

    return alloc;

}

void _co_time_heap_destroy(_co_time_heap_t *h) {
    if(h) {
        if(h->heap) {
            free(h->heap);
        }
        free(h);
    }
}

int _co_time_heap_expand(_co_time_heap_t *h) {
    size_t i;
    _co_time_heap_node_t *alloc;
    if(!(alloc = (_co_time_heap_node_t *)malloc(sizeof(_co_time_heap_node_t) * h->cap * 2))) {
        return -1;
    }
    for(i = 0; i < h->size; ++i) {
        alloc[i] = h->heap[i];
        if(alloc[i].co) {
            alloc[i].co->tlelink = &alloc[i];
        }
    }
    free(h->heap);
    h->heap = alloc;
    h->cap *= 2;
    return 0;
}

static void _co_time_heap_adjust_up(_co_time_heap_t *h, size_t loc) {

    size_t parent;
    _co_time_heap_node_t tmp;

    while(loc) {
        parent = (loc - 1) / 2;
        if(h->heap[parent].timeout > h->heap[loc].timeout) {
            tmp = h->heap[parent];
            h->heap[parent] = h->heap[loc];
            h->heap[loc] = tmp;
            if(h->heap[loc].co) {
                h->heap[loc].co->tlelink = &h->heap[loc];
            }
            if(h->heap[parent].co) {
                h->heap[parent].co->tlelink = &h->heap[parent];
            }
        } else {
            break;
        }
        loc = parent;
    }

    return;
}

int _co_time_heap_insert(_co_time_heap_t *h, _co_time_t timeout, _co_thread_t *co) {

    if(h->size == h->cap) {
        if(_co_time_heap_expand(h) != 0) {
            errno = ENOMEM;
            return -1;
        }
    }
    h->heap[h->size].timeout = timeout;
    h->heap[h->size].co = co;
    if(co) {
        co->tlelink = &h->heap[h->size];
    }
    _co_time_heap_adjust_up(h, h->size);
    h->size += 1;
    return 0;

}

static void _co_time_heap_adjust_down(_co_time_heap_t *h, size_t loc) {
    size_t lchild;
    size_t rchild;
    size_t cmp;
    _co_time_heap_node_t tmp;
    while(loc < h->size) {
        lchild = 2 * loc + 1;
        rchild = 2 * loc + 2;
        if(lchild >= h->size) {
            break;
        }
        cmp = lchild;
        if(rchild < h->size) {
            if(h->heap[lchild].timeout > h->heap[rchild].timeout) {
                cmp = rchild;
            }
        }
        if(h->heap[loc].timeout < h->heap[cmp].timeout) {
            break;
        } else {
            tmp = h->heap[loc];
            h->heap[loc] = h->heap[cmp];
            h->heap[cmp] = tmp;
            if(h->heap[loc].co) {
                h->heap[loc].co->tlelink = &h->heap[loc];
            }
            if(h->heap[cmp].co) {
                h->heap[cmp].co->tlelink = &h->heap[cmp];
            }
        }
        loc = cmp;
    }
    return;
}

_co_time_heap_node_t _co_time_heap_delete(_co_time_heap_t *h, _co_time_heap_node_t *node) {

    size_t loc;
    _co_time_heap_node_t ret = {-1, NULL};
    if(_co_time_heap_empty(h)) {
        return ret;
    }
    loc = ((char *)node - (char *)(h->heap)) / sizeof(_co_time_heap_node_t);

    ret.timeout = h->heap[loc].timeout;
    ret.co = h->heap[loc].co;
    if(ret.co) {
        ret.co->tlelink = NULL;
    }
    h->heap[loc].timeout = h->heap[h->size-1].timeout;
    h->heap[loc].co = h->heap[h->size-1].co;
    h->size -= 1;
    if(h->heap[loc].co) {
        h->heap[loc].co->tlelink = &h->heap[loc];
    }
    _co_time_heap_adjust_down(h, loc);
    return ret;

}

int _co_time_heap_empty(_co_time_heap_t *h) {
    if(h->size == 0) {
        return 1;
    }
    return 0;
}

_co_time_heap_node_t _co_time_heap_top(_co_time_heap_t *h) {
    _co_time_heap_node_t node = {-1, NULL};
    if(_co_time_heap_empty(h)) {
        return node;
    }
    return h->heap[0];
}
