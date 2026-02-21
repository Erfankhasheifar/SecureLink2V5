/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 */

#ifndef FREERTOS_H
#define FREERTOS_H

/*
 * Include the compiler and port-specific definitions first.
 * FreeRTOSConfig.h is the user/application level configuration file.
 */
#include <stddef.h>
#include <stdint.h>

/* Application configuration */
#include "FreeRTOSConfig.h"

/* Basic type definitions */
#include "projdefs.h"

/* Pointer-sized integer type */
#ifndef portPOINTER_SIZE_TYPE
    #define portPOINTER_SIZE_TYPE uintptr_t
#endif

/* ---- Version ----------------------------------------------------------- */
#define tskKERNEL_VERSION_NUMBER    "V10.3.1"
#define tskKERNEL_VERSION_MAJOR     10
#define tskKERNEL_VERSION_MINOR     3
#define tskKERNEL_VERSION_BUILD     1

/* ---- Standard include guards ------------------------------------------- */
#define INC_FREERTOS_H

/* ---- Compiler macros --------------------------------------------------- */
#ifndef portFORCE_INLINE
    #define portFORCE_INLINE __attribute__(( always_inline )) static inline
#endif

/* ---- Task function type ------------------------------------------------ */
typedef void (* TaskFunction_t)( void * );

/* ---- Tick type and max delay ------------------------------------------- */
#if ( configUSE_16_BIT_TICKS == 1 )
    typedef uint16_t TickType_t;
    #define portMAX_DELAY    ( TickType_t ) 0xffff
#else
    typedef uint32_t TickType_t;
    #define portMAX_DELAY    ( TickType_t ) 0xffffffffUL
    #define portTICK_TYPE_IS_ATOMIC 1
#endif

/* ---- Default configUSE_16_BIT_TICKS if not defined -------------------- */
#ifndef configUSE_16_BIT_TICKS
    #define configUSE_16_BIT_TICKS 0
#endif

/* ---- Optional feature defaults ---------------------------------------- */
#ifndef configUSE_PREEMPTION
    #error Missing definition: configUSE_PREEMPTION
#endif

#ifndef configUSE_IDLE_HOOK
    #error Missing definition: configUSE_IDLE_HOOK
#endif

#ifndef configUSE_TICK_HOOK
    #error Missing definition: configUSE_TICK_HOOK
#endif

#ifndef configMAX_PRIORITIES
    #error Missing definition: configMAX_PRIORITIES
#endif

#ifndef configMINIMAL_STACK_SIZE
    #error Missing definition: configMINIMAL_STACK_SIZE
#endif

#ifndef configTOTAL_HEAP_SIZE
    #error Missing definition: configTOTAL_HEAP_SIZE
#endif

#ifndef configMAX_TASK_NAME_LEN
    #define configMAX_TASK_NAME_LEN 16
#endif

#ifndef configIDLE_SHOULD_YIELD
    #define configIDLE_SHOULD_YIELD 1
#endif

#ifndef configUSE_CO_ROUTINES
    #define configUSE_CO_ROUTINES 0
#endif

#ifndef configUSE_MUTEXES
    #define configUSE_MUTEXES 0
#endif

#ifndef configUSE_RECURSIVE_MUTEXES
    #define configUSE_RECURSIVE_MUTEXES 0
#endif

#ifndef configUSE_COUNTING_SEMAPHORES
    #define configUSE_COUNTING_SEMAPHORES 0
#endif

#ifndef configUSE_QUEUE_SETS
    #define configUSE_QUEUE_SETS 0
#endif

#ifndef configUSE_APPLICATION_TASK_TAG
    #define configUSE_APPLICATION_TASK_TAG 0
#endif

#ifndef configUSE_NEWLIB_REENTRANT
    #define configUSE_NEWLIB_REENTRANT 0
#endif

#ifndef configUSE_STATS_FORMATTING_FUNCTIONS
    #define configUSE_STATS_FORMATTING_FUNCTIONS 0
#endif

#ifndef configGENERATE_RUN_TIME_STATS
    #define configGENERATE_RUN_TIME_STATS 0
#endif

#ifndef configUSE_TRACE_FACILITY
    #define configUSE_TRACE_FACILITY 0
#endif

#ifndef configUSE_TIMERS
    #define configUSE_TIMERS 0
#endif

#ifndef configUSE_TASK_NOTIFICATIONS
    #define configUSE_TASK_NOTIFICATIONS 1
#endif

#ifndef configTASK_NOTIFICATION_ARRAY_ENTRIES
    #define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#endif

#ifndef configNUM_THREAD_LOCAL_STORAGE_POINTERS
    #define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#endif

#ifndef configQUEUE_REGISTRY_SIZE
    #define configQUEUE_REGISTRY_SIZE 0
#endif

#ifndef configSUPPORT_DYNAMIC_ALLOCATION
    #define configSUPPORT_DYNAMIC_ALLOCATION 1
#endif

#ifndef configSUPPORT_STATIC_ALLOCATION
    #define configSUPPORT_STATIC_ALLOCATION 0
#endif

#ifndef configUSE_MINI_LIST_ITEM
    #define configUSE_MINI_LIST_ITEM 1
#endif

#ifndef configSTACK_DEPTH_TYPE
    #define configSTACK_DEPTH_TYPE uint16_t
#endif

#ifndef configMESSAGE_BUFFER_LENGTH_TYPE
    #define configMESSAGE_BUFFER_LENGTH_TYPE size_t
#endif

#ifndef configHEAP_CLEAR_MEMORY_ON_FREE
    #define configHEAP_CLEAR_MEMORY_ON_FREE 0
#endif

#ifndef configUSE_STREAM_BUFFERS
    #define configUSE_STREAM_BUFFERS 0
#endif

#ifndef configUSE_EVENT_GROUPS
    #define configUSE_EVENT_GROUPS 1
#endif

/* ---- INCLUDE_ defaults ------------------------------------------------- */
#ifndef INCLUDE_vTaskPrioritySet
    #define INCLUDE_vTaskPrioritySet 0
#endif
#ifndef INCLUDE_uxTaskPriorityGet
    #define INCLUDE_uxTaskPriorityGet 0
#endif
#ifndef INCLUDE_vTaskDelete
    #define INCLUDE_vTaskDelete 0
#endif
#ifndef INCLUDE_vTaskSuspend
    #define INCLUDE_vTaskSuspend 0
#endif
#ifndef INCLUDE_vTaskDelayUntil
    #define INCLUDE_vTaskDelayUntil 0
#endif
#ifndef INCLUDE_vTaskDelay
    #define INCLUDE_vTaskDelay 0
#endif
#ifndef INCLUDE_xTaskGetSchedulerState
    #define INCLUDE_xTaskGetSchedulerState 0
#endif
#ifndef INCLUDE_xTaskGetCurrentTaskHandle
    #define INCLUDE_xTaskGetCurrentTaskHandle 0
#endif
#ifndef INCLUDE_uxTaskGetStackHighWaterMark
    #define INCLUDE_uxTaskGetStackHighWaterMark 0
#endif
#ifndef INCLUDE_uxTaskGetStackHighWaterMark2
    #define INCLUDE_uxTaskGetStackHighWaterMark2 0
#endif
#ifndef INCLUDE_xTaskGetIdleTaskHandle
    #define INCLUDE_xTaskGetIdleTaskHandle 0
#endif
#ifndef INCLUDE_eTaskGetState
    #define INCLUDE_eTaskGetState 0
#endif
#ifndef INCLUDE_xTimerPendFunctionCall
    #define INCLUDE_xTimerPendFunctionCall 0
#endif
#ifndef INCLUDE_xTaskAbortDelay
    #define INCLUDE_xTaskAbortDelay 0
#endif
#ifndef INCLUDE_xTaskGetHandle
    #define INCLUDE_xTaskGetHandle 0
#endif
#ifndef INCLUDE_xResumeFromISR
    #define INCLUDE_xResumeFromISR 1
#endif
#ifndef INCLUDE_xTaskResumeFromISR
    #define INCLUDE_xTaskResumeFromISR 1
#endif
#ifndef INCLUDE_xEventGroupSetBitFromISR
    #define INCLUDE_xEventGroupSetBitFromISR 0
#endif

/* ---- portmacro.h inclusion --------------------------------------------- */
#include "portable.h"

/* ---- Deprecated definitions -------------------------------------------- */
#include "deprecated_definitions.h"

/* ---- Heap prototypes --------------------------------------------------- */
#if configSUPPORT_DYNAMIC_ALLOCATION == 1
    void * pvPortMalloc( size_t xSize ) PRIVILEGED_FUNCTION;
    void   vPortFree( void * pv ) PRIVILEGED_FUNCTION;
    void   vPortInitialiseBlocks( void ) PRIVILEGED_FUNCTION;
    size_t xPortGetFreeHeapSize( void ) PRIVILEGED_FUNCTION;
    size_t xPortGetMinimumEverFreeHeapSize( void ) PRIVILEGED_FUNCTION;
#endif

/* ---- Trace hooks defaults ---------------------------------------------- */
#ifndef traceSTART
    #define traceSTART()
#endif
#ifndef traceEND
    #define traceEND()
#endif
#ifndef traceTASK_SWITCHED_IN
    #define traceTASK_SWITCHED_IN()
#endif
#ifndef traceTASK_SWITCHED_OUT
    #define traceTASK_SWITCHED_OUT()
#endif
#ifndef traceQUEUE_CREATE
    #define traceQUEUE_CREATE( pxNewQueue )
#endif
#ifndef traceQUEUE_CREATE_FAILED
    #define traceQUEUE_CREATE_FAILED( ucQueueType )
#endif
#ifndef traceCREATE_MUTEX
    #define traceCREATE_MUTEX( pxNewQueue )
#endif
#ifndef traceCREATE_MUTEX_FAILED
    #define traceCREATE_MUTEX_FAILED()
#endif
#ifndef traceGIVE_MUTEX_RECURSIVE
    #define traceGIVE_MUTEX_RECURSIVE( pxMutex )
#endif
#ifndef traceGIVE_MUTEX_RECURSIVE_FAILED
    #define traceGIVE_MUTEX_RECURSIVE_FAILED( pxMutex )
#endif
#ifndef traceTAKE_MUTEX_RECURSIVE
    #define traceTAKE_MUTEX_RECURSIVE( pxMutex )
#endif
#ifndef traceTAKE_MUTEX_RECURSIVE_FAILED
    #define traceTAKE_MUTEX_RECURSIVE_FAILED( pxMutex )
#endif
#ifndef traceCREATE_COUNTING_SEMAPHORE
    #define traceCREATE_COUNTING_SEMAPHORE()
#endif
#ifndef traceCREATE_COUNTING_SEMAPHORE_FAILED
    #define traceCREATE_COUNTING_SEMAPHORE_FAILED()
#endif
#ifndef traceQUEUE_SEND
    #define traceQUEUE_SEND( pxQueue )
#endif
#ifndef traceQUEUE_SEND_FAILED
    #define traceQUEUE_SEND_FAILED( pxQueue )
#endif
#ifndef traceQUEUE_RECEIVE
    #define traceQUEUE_RECEIVE( pxQueue )
#endif
#ifndef traceQUEUE_PEEK
    #define traceQUEUE_PEEK( pxQueue )
#endif
#ifndef traceQUEUE_PEEK_FAILED
    #define traceQUEUE_PEEK_FAILED( pxQueue )
#endif
#ifndef traceQUEUE_PEEK_FROM_ISR
    #define traceQUEUE_PEEK_FROM_ISR( pxQueue )
#endif
#ifndef traceQUEUE_RECEIVE_FAILED
    #define traceQUEUE_RECEIVE_FAILED( pxQueue )
#endif
#ifndef traceQUEUE_SEND_FROM_ISR
    #define traceQUEUE_SEND_FROM_ISR( pxQueue )
#endif
#ifndef traceQUEUE_SEND_FROM_ISR_FAILED
    #define traceQUEUE_SEND_FROM_ISR_FAILED( pxQueue )
#endif
#ifndef traceQUEUE_RECEIVE_FROM_ISR
    #define traceQUEUE_RECEIVE_FROM_ISR( pxQueue )
#endif
#ifndef traceQUEUE_RECEIVE_FROM_ISR_FAILED
    #define traceQUEUE_RECEIVE_FROM_ISR_FAILED( pxQueue )
#endif
#ifndef traceQUEUE_PEEK_FROM_ISR_FAILED
    #define traceQUEUE_PEEK_FROM_ISR_FAILED( pxQueue )
#endif
#ifndef traceQUEUE_DELETE
    #define traceQUEUE_DELETE( pxQueue )
#endif
#ifndef traceTASK_CREATE
    #define traceTASK_CREATE( pxNewTCB )
#endif
#ifndef traceTASK_CREATE_FAILED
    #define traceTASK_CREATE_FAILED()
#endif
#ifndef traceTASK_DELETE
    #define traceTASK_DELETE( pxTaskToDelete )
#endif
#ifndef traceTASK_DELAY_UNTIL
    #define traceTASK_DELAY_UNTIL( x )
#endif
#ifndef traceTASK_DELAY
    #define traceTASK_DELAY()
#endif
#ifndef traceTASK_PRIORITY_SET
    #define traceTASK_PRIORITY_SET( pxTask, uxNewPriority )
#endif
#ifndef traceTASK_SUSPEND
    #define traceTASK_SUSPEND( pxTaskToSuspend )
#endif
#ifndef traceTASK_RESUME
    #define traceTASK_RESUME( pxTaskToResume )
#endif
#ifndef traceTASK_RESUME_FROM_ISR
    #define traceTASK_RESUME_FROM_ISR( pxTaskToResume )
#endif
#ifndef traceTASK_INCREMENT_TICK
    #define traceTASK_INCREMENT_TICK( xTickCount )
#endif
#ifndef traceTIMER_CREATE
    #define traceTIMER_CREATE( pxNewTimer )
#endif
#ifndef traceTIMER_CREATE_FAILED
    #define traceTIMER_CREATE_FAILED()
#endif
#ifndef traceTIMER_COMMAND_SEND
    #define traceTIMER_COMMAND_SEND( xTimer, xMessageID, xMessageValueValue, xReturn )
#endif
#ifndef traceTIMER_EXPIRED
    #define traceTIMER_EXPIRED( pxTimer )
#endif
#ifndef traceTIMER_COMMAND_RECEIVED
    #define traceTIMER_COMMAND_RECEIVED( pxTimer, xMessageID, xMessageValue )
#endif
#ifndef traceMALLOC
    #define traceMALLOC( pvAddress, uiSize )
#endif
#ifndef traceFREE
    #define traceFREE( pvAddress, uiSize )
#endif
#ifndef traceEVENT_GROUP_CREATE
    #define traceEVENT_GROUP_CREATE( xEventGroup )
#endif
#ifndef traceEVENT_GROUP_CREATE_FAILED
    #define traceEVENT_GROUP_CREATE_FAILED()
#endif
#ifndef traceEVENT_GROUP_SYNC_BLOCK
    #define traceEVENT_GROUP_SYNC_BLOCK( xEventGroup, uxBitsToSet, uxBitsToWaitFor )
#endif
#ifndef traceEVENT_GROUP_SYNC_END
    #define traceEVENT_GROUP_SYNC_END( xEventGroup, uxBitsToSet, uxBitsToWaitFor, xTimeoutOccurred ) ( void ) xTimeoutOccurred
#endif
#ifndef traceEVENT_GROUP_WAIT_BITS_BLOCK
    #define traceEVENT_GROUP_WAIT_BITS_BLOCK( xEventGroup, uxBitsToWaitFor )
#endif
#ifndef traceEVENT_GROUP_WAIT_BITS_END
    #define traceEVENT_GROUP_WAIT_BITS_END( xEventGroup, uxBitsToWaitFor, xTimeoutOccurred ) ( void ) xTimeoutOccurred
#endif
#ifndef traceEVENT_GROUP_CLEAR_BITS
    #define traceEVENT_GROUP_CLEAR_BITS( xEventGroup, uxBitsToClear )
#endif
#ifndef traceEVENT_GROUP_CLEAR_BITS_FROM_ISR
    #define traceEVENT_GROUP_CLEAR_BITS_FROM_ISR( xEventGroup, uxBitsToClear )
#endif
#ifndef traceEVENT_GROUP_SET_BITS
    #define traceEVENT_GROUP_SET_BITS( xEventGroup, uxBitsToSet )
#endif
#ifndef traceEVENT_GROUP_SET_BITS_FROM_ISR
    #define traceEVENT_GROUP_SET_BITS_FROM_ISR( xEventGroup, uxBitsToSet )
#endif
#ifndef traceEVENT_GROUP_DELETE
    #define traceEVENT_GROUP_DELETE( xEventGroup )
#endif
#ifndef tracePEND_FUNC_CALL
    #define tracePEND_FUNC_CALL(xFunctionToPend, pvParameter1, ulParameter2, ret)
#endif
#ifndef tracePEND_FUNC_CALL_FROM_ISR
    #define tracePEND_FUNC_CALL_FROM_ISR(xFunctionToPend, pvParameter1, ulParameter2, ret)
#endif
#ifndef traceQUEUE_REGISTRY_ADD
    #define traceQUEUE_REGISTRY_ADD(xQueue, pcQueueName)
#endif
#ifndef traceTASK_NOTIFY_TAKE_BLOCK
    #define traceTASK_NOTIFY_TAKE_BLOCK( uxIndexToWait )
#endif
#ifndef traceTASK_NOTIFY_TAKE
    #define traceTASK_NOTIFY_TAKE( uxIndexToWait )
#endif
#ifndef traceTASK_NOTIFY_WAIT_BLOCK
    #define traceTASK_NOTIFY_WAIT_BLOCK( uxIndexToWait )
#endif
#ifndef traceTASK_NOTIFY_WAIT
    #define traceTASK_NOTIFY_WAIT( uxIndexToWait )
#endif
#ifndef traceTASK_NOTIFY
    #define traceTASK_NOTIFY( uxIndexToNotify )
#endif
#ifndef traceTASK_NOTIFY_FROM_ISR
    #define traceTASK_NOTIFY_FROM_ISR( uxIndexToNotify )
#endif
#ifndef traceTASK_NOTIFY_GIVE_FROM_ISR
    #define traceTASK_NOTIFY_GIVE_FROM_ISR( uxIndexToNotify )
#endif
#ifndef traceSTREAM_BUFFER_CREATE_FAILED
    #define traceSTREAM_BUFFER_CREATE_FAILED( xIsMessageBuffer )
#endif
#ifndef traceSTREAM_BUFFER_CREATE_STATIC_FAILED
    #define traceSTREAM_BUFFER_CREATE_STATIC_FAILED( xReturn, xIsMessageBuffer )
#endif
#ifndef traceSTREAM_BUFFER_CREATE
    #define traceSTREAM_BUFFER_CREATE( pxStreamBuffer, xIsMessageBuffer )
#endif
#ifndef traceSTREAM_BUFFER_DELETE
    #define traceSTREAM_BUFFER_DELETE( xStreamBuffer )
#endif
#ifndef traceSTREAM_BUFFER_RESET
    #define traceSTREAM_BUFFER_RESET( xStreamBuffer )
#endif
#ifndef traceBLOCKING_ON_STREAM_BUFFER_SEND
    #define traceBLOCKING_ON_STREAM_BUFFER_SEND( xStreamBuffer )
#endif
#ifndef traceSTREAM_BUFFER_SEND
    #define traceSTREAM_BUFFER_SEND( xStreamBuffer, xBytesSent )
#endif
#ifndef traceSTREAM_BUFFER_SEND_FAILED
    #define traceSTREAM_BUFFER_SEND_FAILED( xStreamBuffer )
#endif
#ifndef traceSTREAM_BUFFER_SEND_FROM_ISR
    #define traceSTREAM_BUFFER_SEND_FROM_ISR( xStreamBuffer, xBytesSent )
#endif
#ifndef traceBLOCKING_ON_STREAM_BUFFER_RECEIVE
    #define traceBLOCKING_ON_STREAM_BUFFER_RECEIVE( xStreamBuffer )
#endif
#ifndef traceSTREAM_BUFFER_RECEIVE
    #define traceSTREAM_BUFFER_RECEIVE( xStreamBuffer, xBytesReceived )
#endif
#ifndef traceSTREAM_BUFFER_RECEIVE_FAILED
    #define traceSTREAM_BUFFER_RECEIVE_FAILED( xStreamBuffer )
#endif
#ifndef traceSTREAM_BUFFER_RECEIVE_FROM_ISR
    #define traceSTREAM_BUFFER_RECEIVE_FROM_ISR( xStreamBuffer, xBytesReceived )
#endif

/* ---- Interrupt handler mapping ----------------------------------------- */
#ifndef vPortSVCHandler
    #define vPortSVCHandler SVC_Handler
#endif
#ifndef xPortPendSVHandler
    #define xPortPendSVHandler PendSV_Handler
#endif
#ifndef xPortSysTickHandler
    #define xPortSysTickHandler SysTick_Handler
#endif

/* ---- configASSERT default ---------------------------------------------- */
#ifndef configASSERT
    #define configASSERT( x )
#endif

#endif /* FREERTOS_H */
