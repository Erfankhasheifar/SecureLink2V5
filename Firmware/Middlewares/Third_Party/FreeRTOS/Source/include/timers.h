/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef TIMERS_H
#define TIMERS_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before timers.h"
#endif

#include "task.h"

/* ---- Timer handle ------------------------------------------------------ */
struct tmrTimerControl;
typedef struct tmrTimerControl * TimerHandle_t;

/* ---- Timer callback function type -------------------------------------- */
typedef void (* TimerCallbackFunction_t)( TimerHandle_t xTimer );

/* ---- Pending function type --------------------------------------------- */
typedef void (* PendedFunction_t)( void *, uint32_t );

/* ---- Timer command IDs ------------------------------------------------- */
#define tmrCOMMAND_EXECUTE_CALLBACK_FROM_ISR    ( ( BaseType_t ) -2 )
#define tmrCOMMAND_EXECUTE_CALLBACK             ( ( BaseType_t ) -1 )
#define tmrCOMMAND_START_DONT_TRACE             ( ( BaseType_t ) 0 )
#define tmrCOMMAND_START                        ( ( BaseType_t ) 1 )
#define tmrCOMMAND_RESET                        ( ( BaseType_t ) 2 )
#define tmrCOMMAND_STOP                         ( ( BaseType_t ) 3 )
#define tmrCOMMAND_CHANGE_PERIOD                ( ( BaseType_t ) 4 )
#define tmrCOMMAND_DELETE                       ( ( BaseType_t ) 5 )
#define tmrFIRST_FROM_ISR_COMMAND               ( ( BaseType_t ) 6 )
#define tmrCOMMAND_START_FROM_ISR               ( ( BaseType_t ) 6 )
#define tmrCOMMAND_RESET_FROM_ISR               ( ( BaseType_t ) 7 )
#define tmrCOMMAND_STOP_FROM_ISR                ( ( BaseType_t ) 8 )
#define tmrCOMMAND_CHANGE_PERIOD_FROM_ISR       ( ( BaseType_t ) 9 )

/* ---- API macros -------------------------------------------------------- */
#define xTimerCreate( pcTimerName, xTimerPeriodInTicks, uxAutoReload, \
                      pvTimerID, pxCallbackFunction )                  \
    xTimerGenericCreate( ( pcTimerName ), ( xTimerPeriodInTicks ),     \
                         ( uxAutoReload ), ( pvTimerID ),              \
                         ( pxCallbackFunction ) )

#define xTimerStart( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_START, \
                          ( xTaskGetTickCount() ), NULL, ( xTicksToWait ) )

#define xTimerStop( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_STOP, 0U, NULL, ( xTicksToWait ) )

#define xTimerChangePeriod( xTimer, xNewPeriod, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_CHANGE_PERIOD, \
                          ( xNewPeriod ), NULL, ( xTicksToWait ) )

#define xTimerDelete( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_DELETE, 0U, NULL, ( xTicksToWait ) )

#define xTimerReset( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_RESET, \
                          ( xTaskGetTickCount() ), NULL, ( xTicksToWait ) )

#define xTimerStartFromISR( xTimer, pxHigherPriorityTaskWoken ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_START_FROM_ISR, \
                          ( xTaskGetTickCountFromISR() ), \
                          ( pxHigherPriorityTaskWoken ), 0U )

#define xTimerStopFromISR( xTimer, pxHigherPriorityTaskWoken ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_STOP_FROM_ISR, \
                          0, ( pxHigherPriorityTaskWoken ), 0U )

#define xTimerChangePeriodFromISR( xTimer, xNewPeriod, pxHigherPriorityTaskWoken ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_CHANGE_PERIOD_FROM_ISR, \
                          ( xNewPeriod ), ( pxHigherPriorityTaskWoken ), 0U )

#define xTimerResetFromISR( xTimer, pxHigherPriorityTaskWoken ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_RESET_FROM_ISR, \
                          ( xTaskGetTickCountFromISR() ), \
                          ( pxHigherPriorityTaskWoken ), 0U )

/* ---- API prototypes ---------------------------------------------------- */
TimerHandle_t xTimerGenericCreate( const char * const pcTimerName,
                                   const TickType_t xTimerPeriodInTicks,
                                   const UBaseType_t uxAutoReload,
                                   void * const pvTimerID,
                                   TimerCallbackFunction_t pxCallbackFunction );

void * pvTimerGetTimerID( const TimerHandle_t xTimer );
void vTimerSetTimerID( TimerHandle_t xTimer, void * pvNewID );
BaseType_t xTimerIsTimerActive( TimerHandle_t xTimer );
TaskHandle_t xTimerGetTimerDaemonTaskHandle( void );

BaseType_t xTimerGenericCommand( TimerHandle_t xTimer,
                                 const BaseType_t xCommandID,
                                 const TickType_t xOptionalValue,
                                 BaseType_t * const pxHigherPriorityTaskWoken,
                                 const TickType_t xTicksToWait );

const char * pcTimerGetName( TimerHandle_t xTimer );
void vTimerSetReloadMode( TimerHandle_t xTimer, const UBaseType_t uxAutoReload );
UBaseType_t uxTimerGetReloadMode( TimerHandle_t xTimer );
TickType_t xTimerGetPeriod( TimerHandle_t xTimer );
TickType_t xTimerGetExpiryTime( TimerHandle_t xTimer );

void vTimerSetTimerNumber( TimerHandle_t xTimer, UBaseType_t uxTimerNumber );
UBaseType_t uxTimerGetTimerNumber( TimerHandle_t xTimer );

#if ( INCLUDE_xTimerPendFunctionCall == 1 )
    BaseType_t xTimerPendFunctionCall( PendedFunction_t xFunctionToPend,
                                       void * pvParameter1,
                                       uint32_t ulParameter2,
                                       TickType_t xTicksToWait );

    BaseType_t xTimerPendFunctionCallFromISR( PendedFunction_t xFunctionToPend,
                                              void * pvParameter1,
                                              uint32_t ulParameter2,
                                              BaseType_t * pxHigherPriorityTaskWoken );
#endif

/* Internal function used by tasks.c */
void vTimerSetTimerListsAndQueue( void );
BaseType_t xTimerCreateTimerTask( void );

#endif /* TIMERS_H */
