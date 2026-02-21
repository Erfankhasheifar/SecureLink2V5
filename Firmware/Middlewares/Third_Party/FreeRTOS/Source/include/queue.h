/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef QUEUE_H
#define QUEUE_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before queue.h"
#endif

/* ---- Queue handle ------------------------------------------------------ */
struct QueueDefinition;
typedef struct QueueDefinition * QueueHandle_t;
typedef struct QueueDefinition * QueueSetHandle_t;
typedef struct QueueDefinition * QueueSetMemberHandle_t;

/* ---- Queue types ------------------------------------------------------- */
#define queueQUEUE_TYPE_BASE                ( ( uint8_t ) 0U )
#define queueQUEUE_TYPE_SET                 ( ( uint8_t ) 0U )
#define queueQUEUE_TYPE_MUTEX               ( ( uint8_t ) 1U )
#define queueQUEUE_TYPE_COUNTING_SEMAPHORE  ( ( uint8_t ) 2U )
#define queueQUEUE_TYPE_BINARY_SEMAPHORE    ( ( uint8_t ) 3U )
#define queueQUEUE_TYPE_RECURSIVE_MUTEX     ( ( uint8_t ) 4U )

/* ---- Send/receive directions ------------------------------------------- */
#define queueSEND_TO_BACK       ( ( BaseType_t ) 0 )
#define queueSEND_TO_FRONT      ( ( BaseType_t ) 1 )
#define queueOVERWRITE          ( ( BaseType_t ) 2 )

/* ---- Convenience macros ------------------------------------------------ */
#define xQueueCreate( uxQueueLength, uxItemSize ) \
    xQueueGenericCreate( ( uxQueueLength ), ( uxItemSize ), ( queueQUEUE_TYPE_BASE ) )

#define xQueueSendToFront( xQueue, pvItemToQueue, xTicksToWait ) \
    xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), ( xTicksToWait ), queueSEND_TO_FRONT )

#define xQueueSendToBack( xQueue, pvItemToQueue, xTicksToWait ) \
    xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), ( xTicksToWait ), queueSEND_TO_BACK )

#define xQueueSend( xQueue, pvItemToQueue, xTicksToWait ) \
    xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), ( xTicksToWait ), queueSEND_TO_BACK )

#define xQueueOverwrite( xQueue, pvItemToQueue ) \
    xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), 0, queueOVERWRITE )

#define xQueueSendToFrontFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken ) \
    xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueSEND_TO_FRONT )

#define xQueueSendToBackFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken ) \
    xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueSEND_TO_BACK )

#define xQueueOverwriteFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken ) \
    xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueOVERWRITE )

#define xQueueSendFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken ) \
    xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueSEND_TO_BACK )

/* ---- API prototypes ---------------------------------------------------- */
QueueHandle_t xQueueGenericCreate( const UBaseType_t uxQueueLength,
                                   const UBaseType_t uxItemSize,
                                   const uint8_t ucQueueType );

BaseType_t xQueueGenericSend( QueueHandle_t xQueue,
                              const void * const pvItemToQueue,
                              TickType_t xTicksToWait,
                              const BaseType_t xCopyPosition );

BaseType_t xQueuePeek( QueueHandle_t xQueue,
                       void * const pvBuffer,
                       TickType_t xTicksToWait );

BaseType_t xQueuePeekFromISR( QueueHandle_t xQueue,
                               void * const pvBuffer );

BaseType_t xQueueReceive( QueueHandle_t xQueue,
                          void * const pvBuffer,
                          TickType_t xTicksToWait );

UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t xQueue );
UBaseType_t uxQueueSpacesAvailable( const QueueHandle_t xQueue );
void vQueueDelete( QueueHandle_t xQueue );

BaseType_t xQueueGenericSendFromISR( QueueHandle_t xQueue,
                                     const void * const pvItemToQueue,
                                     BaseType_t * const pxHigherPriorityTaskWoken,
                                     const BaseType_t xCopyPosition );

BaseType_t xQueueGiveFromISR( QueueHandle_t xQueue,
                               BaseType_t * const pxHigherPriorityTaskWoken );

BaseType_t xQueueReceiveFromISR( QueueHandle_t xQueue,
                                 void * const pvBuffer,
                                 BaseType_t * const pxHigherPriorityTaskWoken );

BaseType_t xQueueIsQueueEmptyFromISR( const QueueHandle_t xQueue );
BaseType_t xQueueIsQueueFullFromISR( const QueueHandle_t xQueue );
UBaseType_t uxQueueMessagesWaitingFromISR( const QueueHandle_t xQueue );

BaseType_t xQueueCRSendFromISR( QueueHandle_t xQueue,
                                 const void *pvItemToQueue,
                                 BaseType_t xCoRoutinePreviouslyWoken );

BaseType_t xQueueCRReceiveFromISR( QueueHandle_t xQueue,
                                    void *pvBuffer,
                                    BaseType_t *pxTaskWoken );

BaseType_t xQueueCRSend( QueueHandle_t xQueue,
                          const void *pvItemToQueue,
                          TickType_t xTicksToWait );

BaseType_t xQueueCRReceive( QueueHandle_t xQueue,
                             void *pvBuffer,
                             TickType_t xTicksToWait );

/* ---- Mutex create ------------------------------------------------------ */
QueueHandle_t xQueueCreateMutex( const uint8_t ucQueueType );
QueueHandle_t xQueueCreateCountingSemaphore( const UBaseType_t uxMaxCount,
                                              const UBaseType_t uxInitialCount );
BaseType_t xQueueSemaphoreTake( QueueHandle_t xQueue,
                                 TickType_t xTicksToWait );
BaseType_t xQueueGetMutexHolder( QueueHandle_t xSemaphore );
BaseType_t xQueueGetMutexHolderFromISR( QueueHandle_t xSemaphore );
BaseType_t xQueueTakeMutexRecursive( QueueHandle_t xMutex,
                                      TickType_t xTicksToWait );
BaseType_t xQueueGiveMutexRecursive( QueueHandle_t xMutex );

/* ---- Registry ---------------------------------------------------------- */
#if ( configQUEUE_REGISTRY_SIZE > 0 )
    void vQueueAddToRegistry( QueueHandle_t xQueue, const char *pcQueueName );
    void vQueueUnregisterQueue( QueueHandle_t xQueue );
    const char * pcQueueGetName( QueueHandle_t xQueue );
#endif

/* ---- Queue sets -------------------------------------------------------- */
#if ( configUSE_QUEUE_SETS == 1 )
    QueueSetHandle_t xQueueCreateSet( const UBaseType_t uxEventQueueLength );
    BaseType_t xQueueAddToSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                                QueueSetHandle_t xQueueSet );
    BaseType_t xQueueRemoveFromSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                                    QueueSetHandle_t xQueueSet );
    QueueSetMemberHandle_t xQueueSelectFromSet( QueueSetHandle_t xQueueSet,
                                                TickType_t const xTicksToWait );
    QueueSetMemberHandle_t xQueueSelectFromSetFromISR( QueueSetHandle_t xQueueSet );
#endif

/* ---- Internal kernel functions ----------------------------------------- */
void vQueueWaitForMessageRestricted( QueueHandle_t xQueue,
                                      TickType_t xTicksToWait,
                                      const BaseType_t xWaitIndefinitely );
BaseType_t xQueueGenericReset( QueueHandle_t xQueue, BaseType_t xNewQueue );

#endif /* QUEUE_H */
