#ifndef MEMORY_MANAGER_QUEUE_H
#define MEMORY_MANAGER_QUEUE_H

#include <inttypes.h>
#include <malloc.h>
#include <memory.h>

typedef struct
{
    int8_t type;
    uint8_t data [256];
} queue_item;

typedef struct circular_buffer
{
    void *buffer;     // data buffer
    void *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void *head;       // pointer to head
    void *tail;       // pointer to tail
} circular_buffer;

void cb_init(circular_buffer *cb, size_t capacity, size_t sz);
void cb_free(circular_buffer *cb);
int cb_push_back(circular_buffer *cb, const void *item);
int cb_pop_front(circular_buffer *cb, void *item);



#endif //MEMORY_MANAGER_QUEUE_H
