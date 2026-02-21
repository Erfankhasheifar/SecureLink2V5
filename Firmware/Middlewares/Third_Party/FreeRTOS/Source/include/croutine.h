/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef CO_ROUTINE_H
#define CO_ROUTINE_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before croutine.h"
#endif

#include "list.h"

#if ( configUSE_CO_ROUTINES != 0 )

typedef void * CoRoutineHandle_t;
typedef void (* crCOROUTINE_CODE)( CoRoutineHandle_t, UBaseType_t );

typedef struct corCoRoutineControlBlock
{
    crCOROUTINE_CODE    pxCoRoutineFunction;
    ListItem_t          xGenericListItem;
    ListItem_t          xEventListItem;
    UBaseType_t         uxPriority;
    UBaseType_t         uxIndex;
    uint16_t            uxState;
} CRCB_t;

BaseType_t xCoRoutineCreate( crCOROUTINE_CODE pxCoRoutineCode,
                             UBaseType_t uxPriority,
                             UBaseType_t uxIndex );

void vCoRoutineSchedule( void );

#define crSTART( pxCRCB ) \
    switch( ( ( CRCB_t * )( pxCRCB ) )->uxState ) { case 0:

#define crEND() }

#define crSET_STATE0( xHandle ) \
    ( ( CRCB_t * )( xHandle ) )->uxState = __LINE__; return; case __LINE__:

#define crDELAY( xHandle, xTicksToDelay ) \
    if( ( xTicksToDelay ) > 0 ) \
    { \
        vCoRoutineAddToDelayedList( ( xTicksToDelay ), NULL ); \
    } \
    crSET_STATE0( ( xHandle ) );

BaseType_t xCoRoutineCreate( crCOROUTINE_CODE pxCoRoutineCode,
                             UBaseType_t uxPriority,
                             UBaseType_t uxIndex );

void vCoRoutineAddToDelayedList( TickType_t xTicksToDelay,
                                  List_t * pxEventList );

BaseType_t xCoRoutineRemoveFromEventList( const List_t * pxEventList );

#endif /* configUSE_CO_ROUTINES */

#endif /* CO_ROUTINE_H */
