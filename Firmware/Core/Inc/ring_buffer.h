/**
 * @file ring_buffer.h
 * @brief Lock-free single-producer / single-consumer ring buffer for the
 *        FT2232HL FIFO bridge running on STM32H750.
 *
 * The buffer lives in AXI-SRAM (D1 domain), which is cache-coherent through
 * the Cortex-M7 D-cache.  Because only ONE task writes and ONE task reads, no
 * critical section is required – the head/tail indices are declared volatile so
 * the compiler always re-reads them from memory.
 *
 * Cache note (STM32H7):
 *   The STM32H7 enables the D-cache by default in CubeMX projects.  AXI-SRAM
 *   (0x24000000) is cached through the MPU with a Write-Back / Read-Allocate
 *   policy, so normal reads/writes are coherent.  If you ever DMA into this
 *   buffer you MUST either:
 *     a) Place the buffer in a non-cacheable MPU region, OR
 *     b) Call SCB_InvalidateDCache_by_Addr() before reading transferred data.
 *   This driver does NOT use DMA, so no extra cache maintenance is needed.
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define RING_BUFFER_SIZE  4096u   /**< Must be a power of two */
#define RING_BUFFER_MASK  (RING_BUFFER_SIZE - 1u)

typedef struct {
    uint8_t  buf[RING_BUFFER_SIZE];
    volatile uint32_t head;  /**< Written by producer */
    volatile uint32_t tail;  /**< Written by consumer */
} ring_buffer_t;

/**
 * @brief Initialise a ring buffer (zero head and tail).
 */
static inline void rb_init(ring_buffer_t *rb)
{
    rb->head = 0u;
    rb->tail = 0u;
}

/**
 * @brief Return the number of bytes currently stored.
 */
static inline uint32_t rb_count(const ring_buffer_t *rb)
{
    return (rb->head - rb->tail) & RING_BUFFER_MASK;
}

/**
 * @brief Return the number of free bytes.
 */
static inline uint32_t rb_free(const ring_buffer_t *rb)
{
    return RING_BUFFER_MASK - rb_count(rb);
}

/**
 * @brief Return true when the buffer is empty.
 */
static inline bool rb_empty(const ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}

/**
 * @brief Return true when the buffer is full.
 */
static inline bool rb_full(const ring_buffer_t *rb)
{
    return rb_free(rb) == 0u;
}

/**
 * @brief Push one byte.  Returns false if the buffer is full.
 */
static inline bool rb_push(ring_buffer_t *rb, uint8_t byte)
{
    if (rb_full(rb)) {
        return false;
    }
    rb->buf[rb->head & RING_BUFFER_MASK] = byte;
    /* Compiler barrier so the store to buf[] is visible before head update */
    __asm volatile ("" ::: "memory");
    rb->head++;
    return true;
}

/**
 * @brief Pop one byte.  Returns false if the buffer is empty.
 */
static inline bool rb_pop(ring_buffer_t *rb, uint8_t *byte)
{
    if (rb_empty(rb)) {
        return false;
    }
    *byte = rb->buf[rb->tail & RING_BUFFER_MASK];
    __asm volatile ("" ::: "memory");
    rb->tail++;
    return true;
}

#endif /* RING_BUFFER_H */
