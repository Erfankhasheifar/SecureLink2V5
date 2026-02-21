/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef TASK_H
#define TASK_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before task.h"
#endif

#include "list.h"

/* ---- Task states ------------------------------------------------------- */
typedef enum
{
    eRunning = 0,
    eReady,
    eBlocked,
    eSuspended,
    eDeleted,
    eInvalid
} eTaskState;

/* ---- Notification actions ---------------------------------------------- */
typedef enum
{
    eNoAction = 0,
    eSetBits,
    eIncrement,
    eSetValueWithOverwrite,
    eSetValueWithoutOverwrite
} eNotifyAction;

/* ---- Task handle ------------------------------------------------------- */
struct tskTaskControlBlock;
typedef struct tskTaskControlBlock * TaskHandle_t;

/* ---- Scheduler states -------------------------------------------------- */
#define taskSCHEDULER_SUSPENDED     ( ( BaseType_t ) 0 )
#define taskSCHEDULER_NOT_STARTED   ( ( BaseType_t ) 1 )
#define taskSCHEDULER_RUNNING       ( ( BaseType_t ) 2 )

/* ---- Task priority macros ---------------------------------------------- */
#define tskIDLE_PRIORITY            ( ( UBaseType_t ) 0U )

/* ---- Yield macros ------------------------------------------------------ */
#define taskYIELD()                 portYIELD()
#define taskENTER_CRITICAL()        portENTER_CRITICAL()
#define taskEXIT_CRITICAL()         portEXIT_CRITICAL()
#define taskENTER_CRITICAL_FROM_ISR()   portSET_INTERRUPT_MASK_FROM_ISR()
#define taskEXIT_CRITICAL_FROM_ISR( x ) portCLEAR_INTERRUPT_MASK_FROM_ISR( x )
#define taskDISABLE_INTERRUPTS()    portDISABLE_INTERRUPTS()
#define taskENABLE_INTERRUPTS()     portENABLE_INTERRUPTS()

/* ---- Task notification values ------------------------------------------ */
#define taskNOTIFICATION_RECEIVED   ( ( uint32_t ) 0x01 )

/* ---- Task status structure --------------------------------------------- */
typedef struct xTASK_STATUS
{
    TaskHandle_t  xHandle;
    const char *  pcTaskName;
    UBaseType_t   xTaskNumber;
    eTaskState    eCurrentState;
    UBaseType_t   uxCurrentPriority;
    UBaseType_t   uxBasePriority;
    uint32_t      ulRunTimeCounter;
    StackType_t * pxStackBase;
    configSTACK_DEPTH_TYPE usStackHighWaterMark;
} TaskStatus_t;

/* ---- Time-out structure ------------------------------------------------ */
typedef struct xTIME_OUT
{
    BaseType_t    xOverflowCount;
    TickType_t    xTimeOnEntering;
} TimeOut_t;

/* ---- Task parameters structure ----------------------------------------- */
typedef struct xTASK_PARAMETERS
{
    TaskFunction_t   pvTaskCode;
    const char *     pcName;
    configSTACK_DEPTH_TYPE usStackDepth;
    void *           pvParameters;
    UBaseType_t      uxPriority;
    StackType_t *    puxStackBuffer;
} TaskParameters_t;

/* ---- Stack fill byte --------------------------------------------------- */
#define tskSTACK_FILL_BYTE   ( 0xa5U )

/* ---- Task creation ----------------------------------------------------- */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                            const char * const pcName,
                            const configSTACK_DEPTH_TYPE usStackDepth,
                            void * const pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t * const pxCreatedTask );
#endif

/* ---- Task deletion ----------------------------------------------------- */
#if ( INCLUDE_vTaskDelete == 1 )
    void vTaskDelete( TaskHandle_t xTaskToDelete );
#endif

/* ---- Task delays ------------------------------------------------------- */
#if ( INCLUDE_vTaskDelay == 1 )
    void vTaskDelay( const TickType_t xTicksToDelay );
#endif

#if ( INCLUDE_vTaskDelayUntil == 1 )
    void vTaskDelayUntil( TickType_t * const pxPreviousWakeTime,
                          const TickType_t xTimeIncrement );
#endif

/* ---- Task priority management ------------------------------------------ */
#if ( INCLUDE_uxTaskPriorityGet == 1 )
    UBaseType_t uxTaskPriorityGet( const TaskHandle_t xTask );
    UBaseType_t uxTaskPriorityGetFromISR( const TaskHandle_t xTask );
#endif

#if ( INCLUDE_vTaskPrioritySet == 1 )
    void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority );
#endif

/* ---- Task suspend/resume ----------------------------------------------- */
#if ( INCLUDE_vTaskSuspend == 1 )
    void vTaskSuspend( TaskHandle_t xTaskToSuspend );
    void vTaskResume( TaskHandle_t xTaskToResume );
    BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume );
#endif

/* ---- Task state query -------------------------------------------------- */
#if ( INCLUDE_eTaskGetState == 1 )
    eTaskState eTaskGetState( TaskHandle_t xTask );
#endif

/* ---- Task handle ------------------------------------------------------- */
#if ( INCLUDE_xTaskGetCurrentTaskHandle == 1 )
    TaskHandle_t xTaskGetCurrentTaskHandle( void );
#endif

#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )
    TaskHandle_t xTaskGetIdleTaskHandle( void );
#endif

#if ( INCLUDE_xTaskGetHandle == 1 )
    TaskHandle_t xTaskGetHandle( const char * pcNameToQuery );
#endif

/* ---- Scheduler control ------------------------------------------------- */
void vTaskStartScheduler( void );
void vTaskEndScheduler( void );
void vTaskSuspendAll( void );
BaseType_t xTaskResumeAll( void );

/* ---- Tick management --------------------------------------------------- */
TickType_t xTaskGetTickCount( void );
TickType_t xTaskGetTickCountFromISR( void );
UBaseType_t uxTaskGetNumberOfTasks( void );
char * pcTaskGetName( TaskHandle_t xTaskToQuery );

/* ---- Scheduler state --------------------------------------------------- */
#if ( INCLUDE_xTaskGetSchedulerState == 1 )
    BaseType_t xTaskGetSchedulerState( void );
#endif

/* ---- Stack watermark --------------------------------------------------- */
#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )
    UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask );
#endif
#if ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 )
    configSTACK_DEPTH_TYPE uxTaskGetStackHighWaterMark2( TaskHandle_t xTask );
#endif

/* ---- Task notification ------------------------------------------------- */
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
    BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify,
                                   UBaseType_t uxIndexToNotify,
                                   uint32_t ulValue,
                                   eNotifyAction eAction,
                                   uint32_t * pulPreviousNotificationValue );

    #define xTaskNotify( xTaskToNotify, ulValue, eAction ) \
        xTaskGenericNotify( ( xTaskToNotify ), ( tskDEFAULT_INDEX_TO_NOTIFY ), \
                            ( ulValue ), ( eAction ), NULL )

    #define xTaskNotifyIndexed( xTaskToNotify, uxIndexToNotify, ulValue, eAction ) \
        xTaskGenericNotify( ( xTaskToNotify ), ( uxIndexToNotify ), \
                            ( ulValue ), ( eAction ), NULL )

    BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify,
                                          UBaseType_t uxIndexToNotify,
                                          uint32_t ulValue,
                                          eNotifyAction eAction,
                                          uint32_t * pulPreviousNotificationValue,
                                          BaseType_t * pxHigherPriorityTaskWoken );

    BaseType_t xTaskGenericNotifyWait( UBaseType_t uxIndexToWaitOn,
                                       uint32_t ulBitsToClearOnEntry,
                                       uint32_t ulBitsToClearOnExit,
                                       uint32_t * pulNotificationValue,
                                       TickType_t xTicksToWait );

    #define xTaskNotifyWait( ulBitsToClearOnEntry, ulBitsToClearOnExit, \
                             pulNotificationValue, xTicksToWait ) \
        xTaskGenericNotifyWait( tskDEFAULT_INDEX_TO_NOTIFY, \
                                ( ulBitsToClearOnEntry ), \
                                ( ulBitsToClearOnExit ), \
                                ( pulNotificationValue ), ( xTicksToWait ) )

    #define xTaskNotifyGive( xTaskToNotify ) \
        xTaskGenericNotify( ( xTaskToNotify ), ( tskDEFAULT_INDEX_TO_NOTIFY ), \
                            ( 0 ), eIncrement, NULL )

    void vTaskGenericNotifyGiveFromISR( TaskHandle_t xTaskToNotify,
                                        UBaseType_t uxIndexToNotify,
                                        BaseType_t * pxHigherPriorityTaskWoken );

    uint32_t ulTaskGenericNotifyTake( UBaseType_t uxIndexToWaitOn,
                                      BaseType_t xClearCountOnExit,
                                      TickType_t xTicksToWait );

    #define ulTaskNotifyTake( xClearCountOnExit, xTicksToWait ) \
        ulTaskGenericNotifyTake( tskDEFAULT_INDEX_TO_NOTIFY, \
                                 ( xClearCountOnExit ), ( xTicksToWait ) )

    BaseType_t xTaskGenericNotifyStateClear( TaskHandle_t xTask,
                                              UBaseType_t uxIndexToClear );

    #define xTaskNotifyStateClear( xTask ) \
        xTaskGenericNotifyStateClear( ( xTask ), ( tskDEFAULT_INDEX_TO_NOTIFY ) )

    uint32_t ulTaskGenericNotifyValueClear( TaskHandle_t xTask,
                                             UBaseType_t uxIndexToClear,
                                             uint32_t ulBitsToClear );

    #define ulTaskNotifyValueClear( xTask, ulBitsToClear ) \
        ulTaskGenericNotifyValueClear( ( xTask ), \
                                       ( tskDEFAULT_INDEX_TO_NOTIFY ), \
                                       ( ulBitsToClear ) )
#endif /* configUSE_TASK_NOTIFICATIONS */

#define tskDEFAULT_INDEX_TO_NOTIFY ( 0 )

/* ---- Internal kernel use ----------------------------------------------- */
BaseType_t xTaskIncrementTick( void );
void vTaskPlaceOnEventList( List_t * const pxEventList,
                             const TickType_t xTicksToWait );
void vTaskPlaceOnUnorderedEventList( List_t * pxEventList,
                                     const TickType_t xItemValue,
                                     const TickType_t xTicksToWait );
void vTaskPlaceOnEventListRestricted( List_t * const pxEventList,
                                       TickType_t xTicksToWait,
                                       const BaseType_t xWaitIndefinitely );
BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList );
void vTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem,
                                        const TickType_t xItemValue );
portDONT_DISCARD void vTaskSwitchContext( void );
TickType_t uxTaskResetEventItemValue( void );
TaskHandle_t pvTaskIncrementMutexHeldCount( void );
void vTaskInternalSetTimeOutState( TimeOut_t * const pxTimeOut );
void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut );
BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut,
                                  TickType_t * const pxTicksToWait );
void vTaskMissedYield( void );
BaseType_t xTaskGetCurrentTaskHandleForCore( BaseType_t xCoreID );
void vTaskSetTaskNumber( TaskHandle_t xTask, const UBaseType_t uxHandle );
UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask );

/* ---- Thread local storage ---------------------------------------------- */
#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
    void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet,
                                            BaseType_t xIndex,
                                            void * pvValue );
    void * pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery,
                                               BaseType_t xIndex );
#endif

/* ---- Task application hook --------------------------------------------- */
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
    void vTaskSetApplicationTaskTag( TaskHandle_t xTask,
                                     TaskHookFunction_t pxTagValue );
    TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask );
    BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask,
                                              void * pvParameter );
#endif

#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                      const UBaseType_t uxArraySize,
                                      uint32_t * const pulTotalRunTime );
#endif

/* ---- Catch abort -------------------------------------------------------- */
#if ( INCLUDE_xTaskAbortDelay == 1 )
    BaseType_t xTaskAbortDelay( TaskHandle_t xTask );
#endif

/* ---- Tick hook wrappers ------------------------------------------------ */
void vApplicationTickHook( void );
void vApplicationIdleHook( void );
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName );

#endif /* TASK_H */
