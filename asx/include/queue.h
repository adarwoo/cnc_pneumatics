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
 * type. For the moment, I have considered a byte as it is the most
 * versatile.
 *
 * The only thing to do before using it is to define the QUEUE_SIZE which
 * will be used for all further queues. That's the price to pay for the
 * speed of static implementations.
 */

#ifndef __EASY_QUEUE_H
#define __EASY_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>
#include <string.h>


typedef struct
{
    void **buffer;
    int8_t head;     	// NOTE: first position with useful data
    int8_t tail;  		// NOTE: first free position
    uint8_t length;
    uint8_t nelem;
} queue_t;

/**
 * @brief Initializes the queue properties and sets the whole array to 0xFF.
 * No queue can be used before initializing.
 *
 * @param queue* Address of the queue you wish to initialize
 * @param nelem Number of elements. If 1, the storage internal
 * @return true if successful
 * @return false otherwise
 */
bool queue_init(queue_t *queue, size_t nelem);

/**
 * @brief Create and initializes the queue properties and
 * sets the whole array to 0xFF.
 *
 * @param nelem Number of elements. If 1, the storage internal
 * @return A pointer to the dynamically allocated queue object
 */
queue_t *queue_create(size_t nelem);

/**
 * @brief Verifies if queue is empty, cpt. Obvious
 *
 * @param queue*
 * @return true when queue is actually empty, or
 * @return false otherwise or when queue is null
 */
bool queue_is_empty(const queue_t *queue);

/**
 * @brief Pushes a single value to the tail (right) side of the queue.
 * When the queue is full, the value will be ignored. If you are
 * looking for a ring buffer behavior, you should try `queue_ringpush()`.
 *
 * @param queue*
 * @param byte the value you need to add to queue
 * @return true when successful, or
 * @return false otherwise
 */
bool queue_push(queue_t *queue, void *data);


/**
 * @brief Pops a single value from the tail (right) side of the queue.
 * When the queue is empty, an error will be returned and the output
 * value will be untouched.
 *
 * @param queue*
 * @param data* the address where to store the popped value
 * @return true when successful, or
 * @return false otherwise
 */
bool queue_pop_front(queue_t *queue, void **data);

/**
 * @brief Pushes a single value to the head (left) side of the queue.
 * When the queue is full, the value will be ignored. If you are
 * looking for a ring buffer like behavior, please remember that the
 * direction should not matter, and a clean implementation should
 * use the `queue_ringpush()` which will add to the tail side.
 *
 * @param queue*
 * @param data
 * @return true
 * @return false
 */
bool queue_push_back(queue_t *queue, void *data);

/**
 * @brief Pops a single value from the head (left) side of the queue.
 * When the queue is empty, an error will be returned and the output
 * value will be untouched.
 *
 * @param queue*
 * @param data* the address where to store the popped value
 * @return true when successful, or
 * @return false otherwise
 */
bool queue_pop(queue_t *queue, void **data);

/**
 * @brief Pushes a single value to the tail (right) side of the queue.
 * When the queue is full, the value will be stored and the head will
 * advance one position, thus loosing the last value. If you are
 * looking for a standard queue buffer behavior, you should try
 * `queue_push()`.
 *
 * @param queue*
 * @param data the value you need to add to queue
 * @return true when successful, or
 * @return false otherwise
 */
void queue_push_ring(queue_t *queue, void *data);

#ifdef __cplusplus
}
#endif

#endif