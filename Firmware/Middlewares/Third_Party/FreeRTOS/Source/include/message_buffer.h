/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before message_buffer.h"
#endif

#include "stream_buffer.h"

typedef StreamBufferHandle_t MessageBufferHandle_t;

#define xMessageBufferCreate( xBufferSizeBytes ) \
    xStreamBufferGenericCreate( ( xBufferSizeBytes ), ( size_t ) 0, pdTRUE )

#define xMessageBufferSend( xMessageBuffer, pvTxData, xDataLengthBytes, xTicksToWait ) \
    xStreamBufferSend( ( xMessageBuffer ), ( pvTxData ), \
                       ( xDataLengthBytes ), ( xTicksToWait ) )

#define xMessageBufferSendFromISR( xMessageBuffer, pvTxData, xDataLengthBytes, \
                                   pxHigherPriorityTaskWoken )                 \
    xStreamBufferSendFromISR( ( xMessageBuffer ), ( pvTxData ), \
                               ( xDataLengthBytes ), ( pxHigherPriorityTaskWoken ) )

#define xMessageBufferReceive( xMessageBuffer, pvRxData, xBufferLengthBytes, xTicksToWait ) \
    xStreamBufferReceive( ( xMessageBuffer ), ( pvRxData ), \
                          ( xBufferLengthBytes ), ( xTicksToWait ) )

#define xMessageBufferReceiveFromISR( xMessageBuffer, pvRxData, xBufferLengthBytes, \
                                      pxHigherPriorityTaskWoken )                   \
    xStreamBufferReceiveFromISR( ( xMessageBuffer ), ( pvRxData ), \
                                  ( xBufferLengthBytes ), ( pxHigherPriorityTaskWoken ) )

#define vMessageBufferDelete( xMessageBuffer ) \
    vStreamBufferDelete( ( xMessageBuffer ) )

#define xMessageBufferIsFull( xMessageBuffer ) \
    xStreamBufferIsFull( ( xMessageBuffer ) )

#define xMessageBufferIsEmpty( xMessageBuffer ) \
    xStreamBufferIsEmpty( ( xMessageBuffer ) )

#define xMessageBufferReset( xMessageBuffer ) \
    xStreamBufferReset( ( xMessageBuffer ) )

#define xMessageBufferSpacesAvailable( xMessageBuffer ) \
    xStreamBufferSpacesAvailable( ( xMessageBuffer ) )

#define xMessageBufferSpaceAvailable( xMessageBuffer ) \
    xStreamBufferSpacesAvailable( ( xMessageBuffer ) )

#define xMessageBufferSendCompletedFromISR( xMessageBuffer, pxHigherPriorityTaskWoken ) \
    xStreamBufferSendCompletedFromISR( ( xMessageBuffer ), ( pxHigherPriorityTaskWoken ) )

#define xMessageBufferReceiveCompletedFromISR( xMessageBuffer, pxHigherPriorityTaskWoken ) \
    xStreamBufferReceiveCompletedFromISR( ( xMessageBuffer ), ( pxHigherPriorityTaskWoken ) )

#endif /* MESSAGE_BUFFER_H */
