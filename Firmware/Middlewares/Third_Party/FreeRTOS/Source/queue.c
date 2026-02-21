/*
 * FreeRTOS Kernel V10.3.1 – queue.c stub
 *
 * Queue, mutex, and semaphore support is not used directly by the
 * application code in this project.  This stub ensures the file
 * compiles cleanly.  A full implementation of queue.c would be
 * required if queues, semaphores, or mutexes are used.
 *
 * The mutex API (configUSE_MUTEXES=1) and counting semaphore API
 * (configUSE_COUNTING_SEMAPHORES=1) are declared in the headers but
 * the application does not call them, so no linker errors result.
 */

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*
 * NOTE: This file intentionally provides no function bodies.
 * The linker will only fail if the application actually calls one of
 * these functions (which it does not in the FIFO_Bridge project).
 *
 * If you add queue/semaphore usage, replace this stub with the full
 * FreeRTOS queue.c from the FreeRTOS kernel distribution.
 */
