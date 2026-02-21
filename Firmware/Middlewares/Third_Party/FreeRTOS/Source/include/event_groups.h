/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef EVENT_GROUPS_H
#define EVENT_GROUPS_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before event_groups.h"
#endif

#include "timers.h"

/* ---- Event bits type --------------------------------------------------- */
#if ( configUSE_16_BIT_TICKS == 1 )
    typedef uint16_t EventBits_t;
#else
    typedef uint32_t EventBits_t;
#endif

/* ---- Event group handle ------------------------------------------------ */
struct EventGroupDef_t;
typedef struct EventGroupDef_t * EventGroupHandle_t;

/* ---- Macros ------------------------------------------------------------ */
#define eventCLEAR_EVENTS_ON_EXIT_BIT   0x01000000UL
#define eventUNBLOCKED_DUE_TO_BIT_SET  0x02000000UL
#define eventWAIT_FOR_ALL_BITS          0x04000000UL
#define eventEVENT_BITS_CONTROL_BYTES   0xff000000UL

/* ---- API prototypes ---------------------------------------------------- */
EventGroupHandle_t xEventGroupCreate( void );
EventBits_t xEventGroupWaitBits( EventGroupHandle_t xEventGroup,
                                  const EventBits_t uxBitsToWaitFor,
                                  const BaseType_t xClearOnExit,
                                  const BaseType_t xWaitForAllBits,
                                  TickType_t xTicksToWait );

EventBits_t xEventGroupClearBits( EventGroupHandle_t xEventGroup,
                                   const EventBits_t uxBitsToClear );

#if ( configUSE_TRACE_FACILITY == 1 )
    BaseType_t xEventGroupClearBitsFromISR( EventGroupHandle_t xEventGroup,
                                             const EventBits_t uxBitsToSet );
#endif

EventBits_t xEventGroupSetBits( EventGroupHandle_t xEventGroup,
                                 const EventBits_t uxBitsToSet );

#if ( ( configUSE_TRACE_FACILITY == 1 ) || \
      ( INCLUDE_xEventGroupSetBitFromISR == 1 ) )
    BaseType_t xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup,
                                           const EventBits_t uxBitsToSet,
                                           BaseType_t * pxHigherPriorityTaskWoken );
#endif

EventBits_t xEventGroupSync( EventGroupHandle_t xEventGroup,
                              const EventBits_t uxBitsToSet,
                              const EventBits_t uxBitsToWaitFor,
                              TickType_t xTicksToWait );

EventBits_t xEventGroupGetBits( EventGroupHandle_t xEventGroup );

#define xEventGroupGetBitsFromISR( xEventGroup ) \
    xEventGroupGetBits( xEventGroup )

void vEventGroupDelete( EventGroupHandle_t xEventGroup );

/* Internal kernel function */
void vEventGroupSetBitsCallback( void * pvEventGroup, uint32_t ulBitsToSet );
void vEventGroupClearBitsCallback( void * pvEventGroup, uint32_t ulBitsToClear );

#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxEventGroupGetNumber( void * xEventGroup );
    void vEventGroupSetNumber( void * xEventGroup, UBaseType_t uxEventGroupNumber );
#endif

#endif /* EVENT_GROUPS_H */
