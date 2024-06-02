/**
 * @brief A typical queue and ring buffer static implementation in ANSI C.
 * Static implementations are really fast, portable (if you're not bothered
 * by compiling) and safe.
 *
 * The implementation cannot be super generic to cover each possible use
 * case, but hopefully its simplicity allows you to easily customize it
 * to your own needs.
 *
 * For example it would worth to parametrize the type of the underlying
 * type. For the moment, I have considered a data as it is the most
 * versatile.
 *
 * The only thing to do before using it is to define the QUEUE_SIZE which
 * will be used for all further queues. That's the price to pay for the
 * speed of static implementations.
 */
#include <assert.h>

#include "queue.h"
#include "mem.h"


bool queue_init(queue_t *queue, size_t nelem)
{
    if (!queue)
    {
        return false;
    }
   
    queue->buffer = (void **)mem_calloc(nelem, sizeof(void *));
    memset( queue->buffer, 0xFF, nelem * sizeof(void *) );
    queue->head = 0;
    queue->tail = 0;
    queue->length = 0;
    queue->nelem = nelem;

    return true;
}

queue_t *queue_create(size_t nelem)
{
    queue_t * queue = (queue_t*)mem_calloc(nelem, sizeof(queue_t));
    
    if ( queue )
    {
        queue_init(queue, nelem);
    }

    return queue;
}


bool queue_is_empty(const queue_t *queue)
{
    assert(queue);

    return queue->length == 0;
}

bool queue_push(queue_t *queue, void *data)
{
    assert(queue);

    if (queue->length >= queue->nelem)
    {
        return false;
    }

    queue->buffer[queue->tail] = data;
    queue->tail = (queue->tail + 1) % queue->nelem;
    ++queue->length;

    return true;
}

bool queue_pop_front(queue_t *queue, void **data)
{
    assert(queue);
    assert(data);

    if (queue->length <= 0)
    {
        return false;
    }

    queue->tail = (queue->tail - 1 + queue->nelem) % queue->nelem;
    *data = queue->buffer[queue->tail];
    --queue->length;

    return true;
}

bool queue_push_back(queue_t *queue, void *data)
{
    assert(queue);

    if (queue->length >= queue->nelem)
    {
        return false;
    }

    queue->head = (queue->head - 1 + queue->nelem) % queue->nelem;
    queue->buffer[queue->head] = data;
    queue->length++;

    return true;
}

bool queue_pop(queue_t *queue, void **data)
{
    assert(queue);
    assert(data);

    if ( queue->length <= 0)
    {
        return false;
    }

    *data = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->nelem;
    queue->length--;

    return true;
}

void queue_push_ring(queue_t *queue, void *data)
{
    assert(queue);

    queue->buffer[queue->tail] = data;
    queue->tail = (queue->tail + 1) % queue->nelem;

    if (queue->length < queue->nelem)
    {
        queue->length++;
    }
    else
    {
        queue->head = (queue->head + 1) % queue->nelem;
    }
}

