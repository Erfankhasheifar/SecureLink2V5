/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef STREAM_BUFFER_H
#define STREAM_BUFFER_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before stream_buffer.h"
#endif

/* ---- Stream buffer handle ---------------------------------------------- */
struct StreamBufferDef_t;
typedef struct StreamBufferDef_t * StreamBufferHandle_t;

/* ---- API macros -------------------------------------------------------- */
#define xStreamBufferCreate( xBufferSizeBytes, xTriggerLevelBytes ) \
    xStreamBufferGenericCreate( ( xBufferSizeBytes ), \
                                ( xTriggerLevelBytes ), pdFALSE )

/* ---- API prototypes ---------------------------------------------------- */
StreamBufferHandle_t xStreamBufferGenericCreate( size_t xBufferSizeBytes,
                                                  size_t xTriggerLevelBytes,
                                                  BaseType_t xIsMessageBuffer );

size_t xStreamBufferSend( StreamBufferHandle_t xStreamBuffer,
                           const void * pvTxData,
                           size_t xDataLengthBytes,
                           TickType_t xTicksToWait );

size_t xStreamBufferSendFromISR( StreamBufferHandle_t xStreamBuffer,
                                  const void * pvTxData,
                                  size_t xDataLengthBytes,
                                  BaseType_t * const pxHigherPriorityTaskWoken );

size_t xStreamBufferReceive( StreamBufferHandle_t xStreamBuffer,
                              void * pvRxData,
                              size_t xBufferLengthBytes,
                              TickType_t xTicksToWait );

size_t xStreamBufferReceiveFromISR( StreamBufferHandle_t xStreamBuffer,
                                     void * pvRxData,
                                     size_t xBufferLengthBytes,
                                     BaseType_t * const pxHigherPriorityTaskWoken );

void vStreamBufferDelete( StreamBufferHandle_t xStreamBuffer );
BaseType_t xStreamBufferIsFull( StreamBufferHandle_t xStreamBuffer );
BaseType_t xStreamBufferIsEmpty( StreamBufferHandle_t xStreamBuffer );
BaseType_t xStreamBufferReset( StreamBufferHandle_t xStreamBuffer );
size_t xStreamBufferSpacesAvailable( StreamBufferHandle_t xStreamBuffer );
size_t xStreamBufferBytesAvailable( StreamBufferHandle_t xStreamBuffer );
BaseType_t xStreamBufferSetTriggerLevel( StreamBufferHandle_t xStreamBuffer,
                                          size_t xTriggerLevel );
BaseType_t xStreamBufferSendCompletedFromISR( StreamBufferHandle_t xStreamBuffer,
                                               BaseType_t * pxHigherPriorityTaskWoken );
BaseType_t xStreamBufferReceiveCompletedFromISR( StreamBufferHandle_t xStreamBuffer,
                                                  BaseType_t * pxHigherPriorityTaskWoken );

#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxStreamBufferGetStreamBufferNumber( StreamBufferHandle_t xStreamBuffer );
    void vStreamBufferSetStreamBufferNumber( StreamBufferHandle_t xStreamBuffer,
                                             UBaseType_t uxStreamBufferNumber );
    uint8_t ucStreamBufferGetStreamBufferType( StreamBufferHandle_t xStreamBuffer );
#endif

#endif /* STREAM_BUFFER_H */
