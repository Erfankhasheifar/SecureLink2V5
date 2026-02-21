/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before semphr.h"
#endif

#include "queue.h"

typedef QueueHandle_t SemaphoreHandle_t;

/* ---- Binary semaphore -------------------------------------------------- */
#define semBINARY_SEMAPHORE_QUEUE_LENGTH    ( ( uint8_t ) 1U )
#define semSEMAPHORE_QUEUE_ITEM_LENGTH      ( ( uint8_t ) 0U )
#define semGIVE_BLOCK_TIME                  ( ( TickType_t ) 0U )

#define xSemaphoreCreateBinary() \
    xQueueGenericCreate( ( UBaseType_t ) 1, \
                         semSEMAPHORE_QUEUE_ITEM_LENGTH, \
                         queueQUEUE_TYPE_BINARY_SEMAPHORE )

#define xSemaphoreTake( xSemaphore, xBlockTime ) \
    xQueueSemaphoreTake( ( xSemaphore ), ( xBlockTime ) )

#define xSemaphoreTakeFromISR( xSemaphore, pxHigherPriorityTaskWoken ) \
    xQueueReceiveFromISR( ( QueueHandle_t ) ( xSemaphore ), \
                          NULL, ( pxHigherPriorityTaskWoken ) )

#define xSemaphoreGive( xSemaphore ) \
    xQueueGenericSend( ( QueueHandle_t ) ( xSemaphore ), \
                       NULL, semGIVE_BLOCK_TIME, queueSEND_TO_BACK )

#define xSemaphoreGiveFromISR( xSemaphore, pxHigherPriorityTaskWoken ) \
    xQueueGiveFromISR( ( QueueHandle_t ) ( xSemaphore ), \
                       ( pxHigherPriorityTaskWoken ) )

#define xSemaphoreGiveRecursive( xMutex ) \
    xQueueGiveMutexRecursive( ( xMutex ) )

#define xSemaphoreTakeRecursive( xMutex, xBlockTime ) \
    xQueueTakeMutexRecursive( ( xMutex ), ( xBlockTime ) )

#define vSemaphoreDelete( xSemaphore ) \
    vQueueDelete( ( QueueHandle_t ) ( xSemaphore ) )

/* ---- Counting semaphore ------------------------------------------------ */
#define xSemaphoreCreateCounting( uxMaxCount, uxInitialCount ) \
    xQueueCreateCountingSemaphore( ( uxMaxCount ), ( uxInitialCount ) )

/* ---- Mutex ------------------------------------------------------------- */
#define xSemaphoreCreateMutex() \
    xQueueCreateMutex( queueQUEUE_TYPE_MUTEX )

#define xSemaphoreCreateRecursiveMutex() \
    xQueueCreateMutex( queueQUEUE_TYPE_RECURSIVE_MUTEX )

#define xSemaphoreGetMutexHolder( xSemaphore ) \
    xQueueGetMutexHolder( ( xSemaphore ) )

#define xSemaphoreGetMutexHolderFromISR( xSemaphore ) \
    xQueueGetMutexHolderFromISR( ( xSemaphore ) )

/* ---- Semaphore count --------------------------------------------------- */
#define uxSemaphoreGetCount( xSemaphore ) \
    uxQueueMessagesWaiting( ( QueueHandle_t ) ( xSemaphore ) )

#define uxSemaphoreGetCountFromISR( xSemaphore ) \
    uxQueueMessagesWaitingFromISR( ( QueueHandle_t ) ( xSemaphore ) )

#endif /* SEMAPHORE_H */
