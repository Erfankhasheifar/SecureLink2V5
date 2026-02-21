/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 *
 * tasks.c – FreeRTOS task scheduler for ARM Cortex-M7 (STM32H750).
 */

/* Standard includes */
#include <stdlib.h>
#include <string.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stack_macros.h"

/* ---- Macros ------------------------------------------------------------ */

#define taskNOT_WAITING_NOTIFICATION    ( ( uint8_t ) 0 )
#define taskWAITING_NOTIFICATION        ( ( uint8_t ) 1 )
#define taskNOTIFICATION_RECEIVED       ( ( uint8_t ) 2 )

#define tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB    ( ( uint8_t ) 0 )
#define tskSTATICALLY_ALLOCATED_STACK_ONLY        ( ( uint8_t ) 1 )
#define tskSTATICALLY_ALLOCATED_STACK_AND_TCB     ( ( uint8_t ) 2 )

#define taskEVENT_LIST_ITEM_VALUE_IN_USE    ( ( TickType_t ) 0x80000000UL )

/* ---- TCB structure ----------------------------------------------------- */

typedef struct tskTaskControlBlock
{
    volatile StackType_t * pxTopOfStack; /**< MUST be first member – used by port.c */

    ListItem_t  xStateListItem;          /**< References the state list the task is in */
    ListItem_t  xEventListItem;          /**< Used to reference a task from an event list */
    UBaseType_t uxPriority;              /**< Priority of the task */
    StackType_t * pxStack;              /**< Points to the start of the task's stack */
    char        pcTaskName[ configMAX_TASK_NAME_LEN ]; /**< Descriptive name */

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )
        volatile uint32_t ulNotifiedValue[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
        volatile uint8_t  ucNotifyState[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
    #endif

    #if ( configUSE_MUTEXES == 1 )
        UBaseType_t uxBasePriority;
        UBaseType_t uxMutexesHeld;
    #endif

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxTCBNumber;
        UBaseType_t uxTaskNumber;
    #endif

    #if ( INCLUDE_vTaskDelete == 1 )
        uint8_t ucDeleted;
    #endif

    uint8_t ucStaticallyAllocated;

} TCB_t;

/* ---- Static task data -------------------------------------------------- */

/* The currently executing task */
TCB_t * volatile pxCurrentTCB = NULL;

/* Ready task lists, one per priority level */
static List_t pxReadyTasksLists[ configMAX_PRIORITIES ];

/* Delayed task list 1 */
static List_t xDelayedTaskList1;
/* Delayed task list 2 (overflow) */
static List_t xDelayedTaskList2;
/* Pointer to the current/overflow delayed list */
static List_t * volatile pxDelayedTaskList;
static List_t * volatile pxOverflowDelayedTaskList;

/* List of tasks waiting to be deleted */
#if ( INCLUDE_vTaskDelete == 1 )
    static List_t xTasksWaitingTermination;
    static volatile UBaseType_t uxDeletedTasksWaitingCleanUp = ( UBaseType_t ) 0U;
#endif

/* List of suspended tasks */
#if ( INCLUDE_vTaskSuspend == 1 )
    static List_t xSuspendedTaskList;
#endif

/* Counters */
static volatile UBaseType_t uxCurrentNumberOfTasks   = ( UBaseType_t ) 0U;
static volatile TickType_t  xTickCount               = ( TickType_t ) configINITIAL_TICK_COUNT;
static volatile UBaseType_t uxTopReadyPriority       = tskIDLE_PRIORITY;
static volatile BaseType_t  xSchedulerRunning        = pdFALSE;
static volatile TickType_t  xNextTaskUnblockTime     = ( TickType_t ) 0U;
static volatile BaseType_t  xNumOfOverflows          = ( BaseType_t ) 0;
static UBaseType_t          uxTaskNumber             = ( UBaseType_t ) 0U;
static TickType_t           xLastTime                = ( TickType_t ) 0U;

/* Suspend/resume support */
static volatile UBaseType_t uxSchedulerSuspended     = ( UBaseType_t ) pdFALSE;
static volatile UBaseType_t uxPendedTicks            = ( UBaseType_t ) 0U;
static volatile BaseType_t  xYieldPending            = pdFALSE;
static volatile BaseType_t  xTaskMissedYield         = pdFALSE;

/* Idle task */
static TCB_t * volatile pxIdleTaskHandle             = NULL;

/* Pending ready list – tasks added here when the scheduler is suspended */
static List_t xPendingReadyList;

/* ---- Counter for deleted tasks ----------------------------------------- */
#if ( configUSE_TRACE_FACILITY == 1 )
    static volatile UBaseType_t uxTraceFacilityTasksDeleted = ( UBaseType_t ) 0U;
#endif

#ifndef configINITIAL_TICK_COUNT
    #define configINITIAL_TICK_COUNT 0
#endif

/* ---- Internal macro helpers -------------------------------------------- */

/* Get the priority from a list item value */
#define taskSELECT_HIGHEST_PRIORITY_TASK()                                        \
    {                                                                             \
        UBaseType_t uxTopPriority = uxTopReadyPriority;                           \
        while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) )    \
        {                                                                         \
            configASSERT( uxTopPriority );                                        \
            --uxTopPriority;                                                      \
        }                                                                         \
        listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB,                               \
                                     &( pxReadyTasksLists[ uxTopPriority ] ) );  \
        uxTopReadyPriority = uxTopPriority;                                       \
    }

#define taskRECORD_READY_PRIORITY( uxPriority )    \
    {                                              \
        if( ( uxPriority ) > uxTopReadyPriority )  \
        {                                          \
            uxTopReadyPriority = ( uxPriority );   \
        }                                          \
    }

#define prvAddTaskToReadyList( pxTCB )                                   \
    traceTASK_MOVED_TASK_TO_READY_STATE( pxTCB );                        \
    taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );                  \
    vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ),     \
                    &( ( pxTCB )->xStateListItem ) )

#define prvResetNextTaskUnblockTime()                           \
    {                                                           \
        if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE ) \
        {                                                       \
            xNextTaskUnblockTime = portMAX_DELAY;               \
        }                                                       \
        else                                                    \
        {                                                       \
            TCB_t * pxTCB;                                      \
            pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList ); \
            xNextTaskUnblockTime = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) ); \
        }                                                       \
    }

/* Trace macros not yet defined in FreeRTOS.h default to nothing */
#ifndef traceTASK_MOVED_TASK_TO_READY_STATE
    #define traceTASK_MOVED_TASK_TO_READY_STATE( pxTCB )
#endif

/* ---- Private prototypes ------------------------------------------------ */
static void prvInitialiseTaskLists( void );
static portTASK_FUNCTION_PROTO( prvIdleTask, pvParameters );
static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait,
                                             const BaseType_t xCanBlockIndefinitely );
static TCB_t * prvAllocateTCBAndStack( const configSTACK_DEPTH_TYPE usStackDepth );

/* ======================================================================== */

static void prvCheckTasksWaitingTermination( void )
{
    #if ( INCLUDE_vTaskDelete == 1 )
    {
        TCB_t * pxTCB;

        while( uxDeletedTasksWaitingCleanUp > ( UBaseType_t ) 0U )
        {
            taskENTER_CRITICAL();
            {
                pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY(
                    ( &xTasksWaitingTermination ) );
                ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                --uxCurrentNumberOfTasks;
                --uxDeletedTasksWaitingCleanUp;
            }
            taskEXIT_CRITICAL();

            vPortFree( pxTCB->pxStack );
            vPortFree( pxTCB );
        }
    }
    #endif /* INCLUDE_vTaskDelete */
}
/* ----------------------------------------------------------------------- */

static TCB_t * prvAllocateTCBAndStack( const configSTACK_DEPTH_TYPE usStackDepth )
{
    TCB_t * pxNewTCB;

    /* Allocate space for the TCB.  Where the memory comes from depends on
     * the implementation of the port malloc function and whether or not static
     * allocation is being used. */
    pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

    if( pxNewTCB != NULL )
    {
        /* Allocate space for the stack used by the task being created.
         * The base of the stack memory stored in the TCB so the task can
         * be deleted later if required. */
        pxNewTCB->pxStack = ( StackType_t * )
                            pvPortMalloc( ( ( ( size_t ) usStackDepth ) *
                                            sizeof( StackType_t ) ) );

        if( pxNewTCB->pxStack == NULL )
        {
            /* Could not allocate the stack.  Delete the allocated TCB. */
            vPortFree( pxNewTCB );
            pxNewTCB = NULL;
        }
        else
        {
            /* Just to help debugging. */
            ( void ) memset( pxNewTCB->pxStack, ( int ) tskSTACK_FILL_BYTE,
                             ( size_t ) usStackDepth * sizeof( StackType_t ) );
        }
    }

    return pxNewTCB;
}
/* ----------------------------------------------------------------------- */

static void prvInitialiseNewTask( TaskFunction_t pxTaskCode,
                                   const char * const pcName,
                                   const configSTACK_DEPTH_TYPE usStackDepth,
                                   void * const pvParameters,
                                   UBaseType_t uxPriority,
                                   TaskHandle_t * const pxCreatedTask,
                                   TCB_t * pxNewTCB )
{
    StackType_t * pxTopOfStack;
    UBaseType_t x;

    /* Calculate the top of stack address. */
    #if ( portSTACK_GROWTH < 0 )
    {
        pxTopOfStack = &( pxNewTCB->pxStack[ usStackDepth - ( configSTACK_DEPTH_TYPE ) 1 ] );
        pxTopOfStack = ( StackType_t * )
                       ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack )
                         & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );

        /* Check the alignment of the calculated top of stack is correct. */
        configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack &
                           ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );
    }
    #else /* portSTACK_GROWTH */
    {
        pxTopOfStack = pxNewTCB->pxStack;

        configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxNewTCB->pxStack &
                           ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );

        pxNewTCB->pxEndOfStack = pxNewTCB->pxStack +
                                 ( usStackDepth - ( configSTACK_DEPTH_TYPE ) 1 );
    }
    #endif /* portSTACK_GROWTH */

    /* Store the task name in the TCB */
    for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
    {
        pxNewTCB->pcTaskName[ x ] = pcName[ x ];
        if( pcName[ x ] == ( char ) 0x00 )
        {
            break;
        }
    }
    pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = '\0';

    /* This is used as an array index so must ensure it's not too large.
     * First remove the privilege bit if one is present. */
    if( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
    {
        uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
    }

    pxNewTCB->uxPriority = uxPriority;

    #if ( configUSE_MUTEXES == 1 )
    {
        pxNewTCB->uxBasePriority = uxPriority;
        pxNewTCB->uxMutexesHeld  = 0;
    }
    #endif

    vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
    vListInitialiseItem( &( pxNewTCB->xEventListItem ) );

    /* Set the pxNewTCB as a link back from the ListItem_t.  This is so we can
     * get back to the containing TCB from a generic item in a list. */
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );

    /* Event lists are always in priority order. */
    listSET_LIST_ITEM_VALUE( &( pxNewTCB->xEventListItem ),
                             ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriority );
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xEventListItem ), pxNewTCB );

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )
    {
        for( x = 0; x < ( UBaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
        {
            pxNewTCB->ulNotifiedValue[ x ] = 0U;
            pxNewTCB->ucNotifyState[ x ]   = taskNOT_WAITING_NOTIFICATION;
        }
    }
    #endif

    #if ( configUSE_TRACE_FACILITY == 1 )
    {
        pxNewTCB->uxTCBNumber = uxTaskNumber;
        uxTaskNumber++;
    }
    #endif

    /* Initialize the new task's top-of-stack pointer. */
    pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack,
                                                     pxTaskCode,
                                                     pvParameters );

    if( pxCreatedTask != NULL )
    {
        *pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
    }
}
/* ----------------------------------------------------------------------- */

static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB )
{
    taskENTER_CRITICAL();
    {
        uxCurrentNumberOfTasks++;

        if( pxCurrentTCB == NULL )
        {
            /* There are no other tasks, or all the other tasks are in the
             * suspended state – make this the current task. */
            pxCurrentTCB = pxNewTCB;

            if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
            {
                /* This is the first task to be created so do the preliminary
                 * initialisation required.  We will not recover if this call
                 * fails, but we will report the failure. */
                prvInitialiseTaskLists();
            }
        }
        else
        {
            /* If the scheduler is not already running, make this task the
             * current task if it is the highest priority task to be created
             * so far. */
            if( xSchedulerRunning == pdFALSE )
            {
                if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
                {
                    pxCurrentTCB = pxNewTCB;
                }
            }
        }

        uxTaskNumber++;

        #if ( configUSE_TRACE_FACILITY == 1 )
        {
            pxNewTCB->uxTaskNumber = uxTaskNumber;
        }
        #endif

        prvAddTaskToReadyList( pxNewTCB );
        traceTASK_CREATE( pxNewTCB );
        portSETUP_TCB( pxNewTCB );
    }
    taskEXIT_CRITICAL();

    if( xSchedulerRunning != pdFALSE )
    {
        /* If the created task is of a higher priority than the current task
         * then it should run now. */
        if( pxCurrentTCB->uxPriority < pxNewTCB->uxPriority )
        {
            taskYIELD_IF_USING_PREEMPTION();
        }
    }
}

#ifndef portSETUP_TCB
    #define portSETUP_TCB( pxTCB ) ( void ) ( pxTCB )
#endif

#ifndef taskYIELD_IF_USING_PREEMPTION
    #if ( configUSE_PREEMPTION == 1 )
        #define taskYIELD_IF_USING_PREEMPTION() portYIELD_WITHIN_API()
    #else
        #define taskYIELD_IF_USING_PREEMPTION()
    #endif
#endif

#ifndef portYIELD_WITHIN_API
    #define portYIELD_WITHIN_API portYIELD
#endif
/* ----------------------------------------------------------------------- */

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                        const char * const pcName,
                        const configSTACK_DEPTH_TYPE usStackDepth,
                        void * const pvParameters,
                        UBaseType_t uxPriority,
                        TaskHandle_t * const pxCreatedTask )
{
    TCB_t * pxNewTCB;
    BaseType_t xReturn;

    pxNewTCB = prvAllocateTCBAndStack( usStackDepth );

    if( pxNewTCB != NULL )
    {
        pxNewTCB->ucStaticallyAllocated = tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB;

        prvInitialiseNewTask( pxTaskCode, pcName, usStackDepth, pvParameters,
                              uxPriority, pxCreatedTask, pxNewTCB );
        prvAddNewTaskToReadyList( pxNewTCB );
        xReturn = pdPASS;
    }
    else
    {
        traceTASK_CREATE_FAILED();
        xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    }

    return xReturn;
}

#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_vTaskDelete == 1 )

void vTaskDelete( TaskHandle_t xTaskToDelete )
{
    TCB_t * pxTCB;

    taskENTER_CRITICAL();
    {
        pxTCB = ( xTaskToDelete == NULL ) ? pxCurrentTCB
                                          : ( TCB_t * ) xTaskToDelete;

        /* Remove task from the ready/delayed/event lists */
        if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
        {
            taskRESET_READY_PRIORITY( pxTCB->uxPriority );
        }

        if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
        {
            ( void ) uxListRemove( &( pxTCB->xEventListItem ) );
        }

        vListInsertEnd( &xTasksWaitingTermination, &( pxTCB->xStateListItem ) );

        ++uxDeletedTasksWaitingCleanUp;
        traceTASK_DELETE( pxTCB );
    }
    taskEXIT_CRITICAL();

    if( pxTCB == pxCurrentTCB )
    {
        portYIELD_WITHIN_API();
    }
}

#endif /* INCLUDE_vTaskDelete */

#ifndef taskRESET_READY_PRIORITY
    #define taskRESET_READY_PRIORITY( uxPriority )               \
    {                                                            \
        if( listCURRENT_LIST_LENGTH(                             \
                &( pxReadyTasksLists[ ( uxPriority ) ] ) ) == 0 ) \
        {                                                        \
            if( ( uxPriority ) == uxTopReadyPriority )           \
            {                                                    \
                ( uxTopReadyPriority )--;                        \
            }                                                    \
        }                                                        \
    }
#endif
/* ----------------------------------------------------------------------- */

static void prvInitialiseTaskLists( void )
{
    UBaseType_t uxPriority;

    for( uxPriority = ( UBaseType_t ) 0U;
         uxPriority < ( UBaseType_t ) configMAX_PRIORITIES;
         uxPriority++ )
    {
        vListInitialise( &( pxReadyTasksLists[ uxPriority ] ) );
    }

    vListInitialise( &xDelayedTaskList1 );
    vListInitialise( &xDelayedTaskList2 );
    vListInitialise( &xPendingReadyList );
    pxDelayedTaskList         = &xDelayedTaskList1;
    pxOverflowDelayedTaskList = &xDelayedTaskList2;

    #if ( INCLUDE_vTaskDelete == 1 )
    {
        vListInitialise( &xTasksWaitingTermination );
    }
    #endif

    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        vListInitialise( &xSuspendedTaskList );
    }
    #endif
}
/* ----------------------------------------------------------------------- */

static portTASK_FUNCTION( prvIdleTask, pvParameters )
{
    ( void ) pvParameters;

    for( ; ; )
    {
        #if ( configUSE_IDLE_HOOK == 1 )
        {
            extern void vApplicationIdleHook( void );
            vApplicationIdleHook();
        }
        #endif

        #if ( INCLUDE_vTaskDelete == 1 )
        {
            prvCheckTasksWaitingTermination();
        }
        #endif

        #if ( configUSE_PREEMPTION == 0 )
        {
            taskYIELD();
        }
        #endif

        #if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
        {
            /* If the idle task is the only task that is ready to run, yield
             * so that any tasks that share the idle task priority can run. */
            if( listCURRENT_LIST_LENGTH(
                    &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) 1 )
            {
                taskYIELD();
            }
        }
        #endif
    }
}
/* ----------------------------------------------------------------------- */

void vTaskStartScheduler( void )
{
    BaseType_t xReturn;

    /* Add the idle task at the lowest priority. */
    #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    {
        xReturn = xTaskCreate( prvIdleTask,
                               configIDLE_TASK_NAME,
                               configMINIMAL_STACK_SIZE,
                               ( void * ) NULL,
                               portPRIVILEGE_BIT,          /* In effect (tskIDLE_PRIORITY | portPRIVILEGE_BIT) */
                               &pxIdleTaskHandle );
    }
    #endif

    #if ( configUSE_TIMERS == 1 )
    {
        if( xReturn == pdPASS )
        {
            xReturn = xTimerCreateTimerTask();
        }
    }
    #endif

    if( xReturn == pdPASS )
    {
        /* freertos_tasks_c_additions_init() is called here if it is defined.
         * This is not used in this project. */

        /* Interrupts are turned off here, to ensure a tick does not occur
         * before or during the call to xPortStartScheduler().  The stacks of
         * the created tasks contain a status word with interrupts switched on
         * so interrupts will automatically get re-enabled when the first task
         * starts to run. */
        portDISABLE_INTERRUPTS();

        xNextTaskUnblockTime = portMAX_DELAY;
        xSchedulerRunning    = pdTRUE;
        xTickCount           = ( TickType_t ) configINITIAL_TICK_COUNT;

        /* If configGENERATE_RUN_TIME_STATS is defined then the following
         * macro must be defined to configure the timer/counter used to
         * generate the run time counter time base.  NOTE:  If
         * configGENERATE_RUN_TIME_STATS is set to 0 and the following line
         * fails to build then ensure you do not have portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
         * defined somewhere in your code. */
        #if ( configGENERATE_RUN_TIME_STATS == 1 )
        {
            portCONFIGURE_TIMER_FOR_RUN_TIME_STATS();
        }
        #endif

        traceSTART();

        /* Setting up the timer tick is hardware specific and thus in the
         * portable interface. */
        if( xPortStartScheduler() != pdFALSE )
        {
            /* Should not reach here as if the scheduler is running the
             * function will not return. */
        }
        else
        {
            /* Should only reach here if a task calls xTaskEndScheduler(). */
        }
    }

    /* This line will only be reached if the kernel could not be started,
     * because there was not enough FreeRTOS heap to create the idle task or
     * the timer task. */
    configASSERT( xReturn != errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY );

    /* Prevent compiler warnings if INCLUDE_xTaskGetIdleTaskHandle is set to 0,
     * meaning pxIdleTaskHandle is not used anywhere else. */
    ( void ) pxIdleTaskHandle;
}

#ifndef configIDLE_TASK_NAME
    #define configIDLE_TASK_NAME "IDLE"
#endif

#ifndef portPRIVILEGE_BIT
    #define portPRIVILEGE_BIT ( ( UBaseType_t ) 0x00 )
#endif
/* ----------------------------------------------------------------------- */

void vTaskEndScheduler( void )
{
    portDISABLE_INTERRUPTS();
    xSchedulerRunning = pdFALSE;
    vPortEndScheduler();
}
/* ----------------------------------------------------------------------- */

void vTaskSuspendAll( void )
{
    portSOFTWARE_BARRIER();
    ++uxSchedulerSuspended;
    portMEMORY_BARRIER();
}

#ifndef portSOFTWARE_BARRIER
    #define portSOFTWARE_BARRIER() __asm volatile( "" ::: "memory" )
#endif
/* ----------------------------------------------------------------------- */

BaseType_t xTaskResumeAll( void )
{
    TCB_t * pxTCB = NULL;
    BaseType_t xAlreadyYielded = pdFALSE;

    /* If uxSchedulerSuspended is zero then this function does not match a
     * previous call to vTaskSuspendAll(). */
    configASSERT( uxSchedulerSuspended );

    /* It is possible that an ISR caused a task to be removed from an event
     * list while the scheduler was suspended.  If this was the case then the
     * removed task will have been added to the xPendingReadyList.  Once the
     * scheduler has been resumed it is safe to move all the pending ready
     * tasks from this list into their appropriate ready list. */
    taskENTER_CRITICAL();
    {
        --uxSchedulerSuspended;

        if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
        {
            if( uxCurrentNumberOfTasks > ( UBaseType_t ) 0U )
            {
                /* Move any readied tasks from the pending list into the
                 * appropriate ready list. */
                while( uxPendedTicks > ( TickType_t ) 0U )
                {
                    if( xTaskIncrementTick() != pdFALSE )
                    {
                        xYieldPending = pdTRUE;
                    }
                    --uxPendedTicks;
                }

                if( xYieldPending != pdFALSE )
                {
                    #if ( configUSE_PREEMPTION != 0 )
                    {
                        xAlreadyYielded = pdTRUE;
                        taskYIELD_IF_USING_PREEMPTION();
                    }
                    #endif
                }
            }
        }
    }
    taskEXIT_CRITICAL();

    return xAlreadyYielded;
}
/* ----------------------------------------------------------------------- */

TickType_t xTaskGetTickCount( void )
{
    TickType_t xTicks;

    /* Critical section required if running on a 16 bit processor. */
    portTICK_TYPE_ENTER_CRITICAL();
    {
        xTicks = xTickCount;
    }
    portTICK_TYPE_EXIT_CRITICAL();

    return xTicks;
}

#ifndef portTICK_TYPE_ENTER_CRITICAL
    #define portTICK_TYPE_ENTER_CRITICAL() portENTER_CRITICAL()
#endif
#ifndef portTICK_TYPE_EXIT_CRITICAL
    #define portTICK_TYPE_EXIT_CRITICAL() portEXIT_CRITICAL()
#endif
/* ----------------------------------------------------------------------- */

TickType_t xTaskGetTickCountFromISR( void )
{
    TickType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;

    portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR();
    {
        xReturn = xTickCount;
    }
    portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

    return xReturn;
}

#ifndef portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR
    #define portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR() \
        UBaseType_t uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR()
#endif
#ifndef portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR
    #define portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR( x ) \
        portCLEAR_INTERRUPT_MASK_FROM_ISR( x )
#endif
/* ----------------------------------------------------------------------- */

UBaseType_t uxTaskGetNumberOfTasks( void )
{
    return uxCurrentNumberOfTasks;
}
/* ----------------------------------------------------------------------- */

char * pcTaskGetName( TaskHandle_t xTaskToQuery )
{
    TCB_t * pxTCB;

    pxTCB = ( xTaskToQuery == NULL ) ? pxCurrentTCB
                                     : ( TCB_t * ) xTaskToQuery;
    configASSERT( pxTCB );
    return &( pxTCB->pcTaskName[ 0 ] );
}
/* ----------------------------------------------------------------------- */

BaseType_t xTaskIncrementTick( void )
{
    TCB_t * pxTCB;
    TickType_t xItemValue;
    BaseType_t xSwitchRequired = pdFALSE;

    traceTASK_INCREMENT_TICK( xTickCount );

    if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
    {
        const TickType_t xConstTickCount = xTickCount + ( TickType_t ) 1;
        xTickCount = xConstTickCount;

        if( xConstTickCount == ( TickType_t ) 0U )
        {
            /* Swap the delayed task lists.  The xDelayedTaskList1 and
             * xDelayedTaskList2 could be swapped rather than just the
             * pointers to them. */
            List_t * const pxTemp = pxDelayedTaskList;
            pxDelayedTaskList     = pxOverflowDelayedTaskList;
            pxOverflowDelayedTaskList = pxTemp;
            xNumOfOverflows++;
            prvResetNextTaskUnblockTime();
        }

        /* See if this tick has made a timeout expire.  Tasks are stored in
         * the queue in the order of their wake time – meaning once one task
         * has been found whose block time has not expired there is no need to
         * look any further down the list. */
        if( xConstTickCount >= xNextTaskUnblockTime )
        {
            for( ; ; )
            {
                if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
                {
                    /* The delayed list is empty.  Set xNextTaskUnblockTime to
                     * the maximum possible value so it is extremely unlikely
                     * that the if() condition above will be true the next time
                     * through. */
                    xNextTaskUnblockTime = portMAX_DELAY;
                    break;
                }
                else
                {
                    /* The delayed list is not empty, get the value of the
                     * item at the head of the delayed list. */
                    pxTCB     = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
                    xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );

                    if( xConstTickCount < xItemValue )
                    {
                        /* It is not time to unblock this item yet, but the
                         * item value is the time at which the task at the
                         * head of the blocked list must be removed from the
                         * Blocked state, so record the item value in
                         * xNextTaskUnblockTime. */
                        xNextTaskUnblockTime = xItemValue;
                        break;
                    }

                    /* It is time to remove the item from the Blocked state. */
                    ( void ) uxListRemove( &( pxTCB->xStateListItem ) );

                    /* Is the task waiting on an event also?  If so remove
                     * it from the event list. */
                    if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
                    {
                        ( void ) uxListRemove( &( pxTCB->xEventListItem ) );
                    }

                    /* Place the unblocked task into the appropriate ready
                     * list. */
                    prvAddTaskToReadyList( pxTCB );

                    /* A task being unblocked cannot cause an immediate
                     * context switch if preemption is turned off. */
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        /* Preemption is on, but a context switch should
                         * only be performed if the unblocked task's
                         * priority is greater than or equal to the current
                         * executing task. */
                        if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
                        {
                            xSwitchRequired = pdTRUE;
                        }
                    }
                    #endif /* configUSE_PREEMPTION */
                }
            }
        }

        /* Tasks of equal priority to the currently running task will share
         * processing time (time slice) if preemption is on, and the
         * application writer has not explicitly turned time slicing off. */
        #if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
        {
            if( listCURRENT_LIST_LENGTH(
                    &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) ) > ( UBaseType_t ) 1 )
            {
                xSwitchRequired = pdTRUE;
            }
        }
        #endif /* ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) ) */

        #if ( configUSE_TICK_HOOK == 1 )
        {
            /* Guard against the tick hook being called when the pended tick
             * count is being unwound (when the scheduler is being unlocked). */
            if( xPendedTicks == ( TickType_t ) 0 )
            {
                vApplicationTickHook();
            }
        }
        #endif
    }
    else
    {
        ++uxPendedTicks;

        #if ( configUSE_TICK_HOOK == 1 )
        {
            vApplicationTickHook();
        }
        #endif
    }

    #if ( configUSE_PREEMPTION == 1 )
    {
        if( xYieldPending != pdFALSE )
        {
            xSwitchRequired = pdTRUE;
        }
    }
    #endif

    return xSwitchRequired;
}
/* ----------------------------------------------------------------------- */

portDONT_DISCARD void vTaskSwitchContext( void )
{
    if( uxSchedulerSuspended != ( UBaseType_t ) pdFALSE )
    {
        xYieldPending = pdTRUE;
    }
    else
    {
        xYieldPending = pdFALSE;
        traceTASK_SWITCHED_OUT();

        #if ( configGENERATE_RUN_TIME_STATS == 1 )
        {
            /* Nothing needed here */
        }
        #endif

        /* Check for stack overflow, if configured. */
        taskFIRST_CHECK_FOR_STACK_OVERFLOW();
        taskSECOND_CHECK_FOR_STACK_OVERFLOW();

        taskSELECT_HIGHEST_PRIORITY_TASK();
        traceTASK_SWITCHED_IN();
    }
}
/* ----------------------------------------------------------------------- */

void vTaskPlaceOnEventList( List_t * const pxEventList,
                             const TickType_t xTicksToWait )
{
    configASSERT( pxEventList );

    /* THIS FUNCTION MUST BE CALLED WITH EITHER INTERRUPTS DISABLED OR THE
     * SCHEDULER SUSPENDED AND THE QUEUE BEING ACCESSED LOCKED. */

    /* Place the event list item of the TCB in the appropriate event list.
     * This is placed in the list in priority order so the highest priority
     * task is the first to be woken by the event. */
    vListInsert( pxEventList, &( pxCurrentTCB->xEventListItem ) );

    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
}
/* ----------------------------------------------------------------------- */

void vTaskPlaceOnUnorderedEventList( List_t * pxEventList,
                                      const TickType_t xItemValue,
                                      const TickType_t xTicksToWait )
{
    configASSERT( pxEventList );
    configASSERT( uxSchedulerSuspended != 0 );

    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ),
                             xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );

    vListInsertEnd( pxEventList, &( pxCurrentTCB->xEventListItem ) );

    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
}
/* ----------------------------------------------------------------------- */

void vTaskPlaceOnEventListRestricted( List_t * const pxEventList,
                                       TickType_t xTicksToWait,
                                       const BaseType_t xWaitIndefinitely )
{
    configASSERT( pxEventList );

    vListInsertEnd( pxEventList, &( pxCurrentTCB->xEventListItem ) );

    prvAddCurrentTaskToDelayedList( xTicksToWait, xWaitIndefinitely );
}
/* ----------------------------------------------------------------------- */

BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList )
{
    TCB_t * const pxUnblockedTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxEventList );
    BaseType_t xReturn;

    configASSERT( pxUnblockedTCB );

    ( void ) uxListRemove( &( pxUnblockedTCB->xEventListItem ) );

    if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
    {
        ( void ) uxListRemove( &( pxUnblockedTCB->xStateListItem ) );
        prvAddTaskToReadyList( pxUnblockedTCB );
    }
    else
    {
        /* Cannot access the delayed or ready lists, so will hold this task
         * pending until the scheduler is resumed. */
        vListInsertEnd( &xPendingReadyList, &( pxUnblockedTCB->xEventListItem ) );
    }

    if( pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority )
    {
        xReturn = pdTRUE;
        xYieldPending = pdTRUE;
    }
    else
    {
        xReturn = pdFALSE;
    }

    return xReturn;
}
/* ----------------------------------------------------------------------- */

void vTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem,
                                        const TickType_t xItemValue )
{
    TCB_t * const pxUnblockedTCB = ( TCB_t * ) listGET_LIST_ITEM_OWNER( pxEventListItem );

    configASSERT( uxSchedulerSuspended != pdFALSE );

    listSET_LIST_ITEM_VALUE( pxEventListItem, xItemValue & ~taskEVENT_LIST_ITEM_VALUE_IN_USE );

    ( void ) uxListRemove( pxEventListItem );
    ( void ) uxListRemove( &( pxUnblockedTCB->xStateListItem ) );
    prvAddTaskToReadyList( pxUnblockedTCB );

    if( pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority )
    {
        xYieldPending = pdTRUE;
    }
}
/* ----------------------------------------------------------------------- */

void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut )
{
    configASSERT( pxTimeOut );
    taskENTER_CRITICAL();
    {
        pxTimeOut->xOverflowCount = xNumOfOverflows;
        pxTimeOut->xTimeOnEntering = xTickCount;
    }
    taskEXIT_CRITICAL();
}
/* ----------------------------------------------------------------------- */

void vTaskInternalSetTimeOutState( TimeOut_t * const pxTimeOut )
{
    pxTimeOut->xOverflowCount = xNumOfOverflows;
    pxTimeOut->xTimeOnEntering = xTickCount;
}
/* ----------------------------------------------------------------------- */

BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut,
                                  TickType_t * const pxTicksToWait )
{
    BaseType_t xReturn;

    configASSERT( pxTimeOut );
    configASSERT( pxTicksToWait );

    taskENTER_CRITICAL();
    {
        const TickType_t xConstTickCount = xTickCount;
        const TickType_t xElapsedTime    = xConstTickCount - pxTimeOut->xTimeOnEntering;

        if( *pxTicksToWait == portMAX_DELAY )
        {
            xReturn = pdFALSE;
        }
        else if( ( xNumOfOverflows != pxTimeOut->xOverflowCount ) &&
                 ( xConstTickCount >= pxTimeOut->xTimeOnEntering ) )
        {
            /* An overflow occurred. Set timeout. */
            xReturn = pdTRUE;
        }
        else if( xElapsedTime < *pxTicksToWait )
        {
            *pxTicksToWait -= xElapsedTime;
            vTaskInternalSetTimeOutState( pxTimeOut );
            xReturn = pdFALSE;
        }
        else
        {
            *pxTicksToWait = 0;
            xReturn = pdTRUE;
        }
    }
    taskEXIT_CRITICAL();

    return xReturn;
}
/* ----------------------------------------------------------------------- */

void vTaskMissedYield( void )
{
    xYieldPending = pdTRUE;
}
/* ----------------------------------------------------------------------- */

TickType_t uxTaskResetEventItemValue( void )
{
    TickType_t uxReturn = listGET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ) );

    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ),
                             ( ( TickType_t ) configMAX_PRIORITIES -
                               ( TickType_t ) pxCurrentTCB->uxPriority ) );

    return uxReturn;
}
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_vTaskDelay == 1 )

void vTaskDelay( const TickType_t xTicksToDelay )
{
    BaseType_t xAlreadyYielded = pdFALSE;

    if( xTicksToDelay > ( TickType_t ) 0U )
    {
        configASSERT( uxSchedulerSuspended == 0 );
        vTaskSuspendAll();
        {
            traceTASK_DELAY();
            prvAddCurrentTaskToDelayedList( xTicksToDelay, pdFALSE );
        }
        xAlreadyYielded = xTaskResumeAll();
    }

    if( xAlreadyYielded == pdFALSE )
    {
        portYIELD_WITHIN_API();
    }
}

#endif /* INCLUDE_vTaskDelay */
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_vTaskDelayUntil == 1 )

void vTaskDelayUntil( TickType_t * const pxPreviousWakeTime,
                      const TickType_t xTimeIncrement )
{
    TickType_t xTimeToWake;
    BaseType_t xAlreadyYielded = pdFALSE;
    BaseType_t xShouldDelay    = pdFALSE;

    configASSERT( pxPreviousWakeTime );
    configASSERT( ( xTimeIncrement > 0U ) );
    configASSERT( uxSchedulerSuspended == 0 );

    vTaskSuspendAll();
    {
        const TickType_t xConstTickCount = xTickCount;

        xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

        if( xConstTickCount < *pxPreviousWakeTime )
        {
            /* Overflow – next wake time hasn't wrapped. */
            if( ( xTimeToWake < *pxPreviousWakeTime ) &&
                ( xTimeToWake > xConstTickCount ) )
            {
                xShouldDelay = pdTRUE;
            }
        }
        else
        {
            if( ( xTimeToWake < *pxPreviousWakeTime ) ||
                ( xTimeToWake > xConstTickCount ) )
            {
                xShouldDelay = pdTRUE;
            }
        }

        *pxPreviousWakeTime = xTimeToWake;

        if( xShouldDelay != pdFALSE )
        {
            traceTASK_DELAY_UNTIL( xTimeToWake );
            prvAddCurrentTaskToDelayedList( xTimeToWake - xConstTickCount, pdFALSE );
        }
    }
    xAlreadyYielded = xTaskResumeAll();

    if( xAlreadyYielded == pdFALSE )
    {
        portYIELD_WITHIN_API();
    }
}

#endif /* INCLUDE_vTaskDelayUntil */
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_vTaskSuspend == 1 )

void vTaskSuspend( TaskHandle_t xTaskToSuspend )
{
    TCB_t * pxTCB;

    taskENTER_CRITICAL();
    {
        pxTCB = ( xTaskToSuspend == NULL ) ? pxCurrentTCB
                                           : ( TCB_t * ) xTaskToSuspend;

        if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
        {
            taskRESET_READY_PRIORITY( pxTCB->uxPriority );
        }

        if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
        {
            ( void ) uxListRemove( &( pxTCB->xEventListItem ) );
        }

        vListInsertEnd( &xSuspendedTaskList, &( pxTCB->xStateListItem ) );

        #if ( configUSE_TASK_NOTIFICATIONS == 1 )
        {
            BaseType_t x;
            for( x = 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
            {
                if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                {
                    pxTCB->ucNotifyState[ x ] = taskNOT_WAITING_NOTIFICATION;
                }
            }
        }
        #endif
    }
    taskEXIT_CRITICAL();

    if( xSchedulerRunning != pdFALSE )
    {
        taskENTER_CRITICAL();
        {
            prvResetNextTaskUnblockTime();
        }
        taskEXIT_CRITICAL();
    }

    if( pxTCB == pxCurrentTCB )
    {
        if( xSchedulerRunning != pdFALSE )
        {
            portYIELD_WITHIN_API();
        }
    }
}

void vTaskResume( TaskHandle_t xTaskToResume )
{
    TCB_t * const pxTCB = ( TCB_t * ) xTaskToResume;

    if( ( pxTCB != pxCurrentTCB ) && ( pxTCB != NULL ) )
    {
        taskENTER_CRITICAL();
        {
            if( listIS_CONTAINED_WITHIN( &xSuspendedTaskList,
                                          &( pxTCB->xStateListItem ) ) != pdFALSE )
            {
                ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                prvAddTaskToReadyList( pxTCB );

                if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
                {
                    taskYIELD_IF_USING_PREEMPTION();
                }
            }
        }
        taskEXIT_CRITICAL();
    }
}

BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume )
{
    BaseType_t xYieldRequired = pdFALSE;
    TCB_t * const pxTCB = ( TCB_t * ) xTaskToResume;
    UBaseType_t uxSavedInterruptStatus;

    configASSERT( xTaskToResume );

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
    {
        if( listIS_CONTAINED_WITHIN( &xSuspendedTaskList,
                                      &( pxTCB->xStateListItem ) ) != pdFALSE )
        {
            ( void ) uxListRemove( &( pxTCB->xStateListItem ) );

            if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
            {
                prvAddTaskToReadyList( pxTCB );
            }
            else
            {
                vListInsertEnd( &xPendingReadyList, &( pxTCB->xEventListItem ) );
            }

            if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
            {
                xYieldRequired = pdTRUE;
                xYieldPending = pdTRUE;
            }
        }
    }
    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

    return xYieldRequired;
}

#endif /* INCLUDE_vTaskSuspend */
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_xTaskGetSchedulerState == 1 )

BaseType_t xTaskGetSchedulerState( void )
{
    BaseType_t xReturn;

    if( xSchedulerRunning == pdFALSE )
    {
        xReturn = taskSCHEDULER_NOT_STARTED;
    }
    else
    {
        if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
        {
            xReturn = taskSCHEDULER_RUNNING;
        }
        else
        {
            xReturn = taskSCHEDULER_SUSPENDED;
        }
    }

    return xReturn;
}

#endif /* INCLUDE_xTaskGetSchedulerState */
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_uxTaskPriorityGet == 1 )

UBaseType_t uxTaskPriorityGet( const TaskHandle_t xTask )
{
    TCB_t const * pxTCB;
    UBaseType_t uxReturn;

    taskENTER_CRITICAL();
    {
        pxTCB = ( xTask == NULL ) ? pxCurrentTCB : ( TCB_t * ) xTask;
        uxReturn = pxTCB->uxPriority;
    }
    taskEXIT_CRITICAL();

    return uxReturn;
}

UBaseType_t uxTaskPriorityGetFromISR( const TaskHandle_t xTask )
{
    TCB_t const * pxTCB;
    UBaseType_t uxReturn;
    UBaseType_t uxSavedInterruptStatus;

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
    {
        pxTCB = ( xTask == NULL ) ? pxCurrentTCB : ( TCB_t * ) xTask;
        uxReturn = pxTCB->uxPriority;
    }
    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

    return uxReturn;
}

#endif /* INCLUDE_uxTaskPriorityGet */
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_vTaskPrioritySet == 1 )

void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority )
{
    TCB_t * pxTCB;
    UBaseType_t uxCurrentBasePriority, uxPriorityUsedOnEntry;
    BaseType_t xYieldRequired = pdFALSE;

    configASSERT( uxNewPriority < configMAX_PRIORITIES );

    if( uxNewPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
    {
        uxNewPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
    }

    taskENTER_CRITICAL();
    {
        pxTCB = ( xTask == NULL ) ? pxCurrentTCB : ( TCB_t * ) xTask;
        traceTASK_PRIORITY_SET( pxTCB, uxNewPriority );

        #if ( configUSE_MUTEXES == 1 )
        {
            uxCurrentBasePriority = pxTCB->uxBasePriority;
        }
        #else
        {
            uxCurrentBasePriority = pxTCB->uxPriority;
        }
        #endif

        if( uxCurrentBasePriority != uxNewPriority )
        {
            if( uxNewPriority > uxCurrentBasePriority )
            {
                if( pxTCB != pxCurrentTCB )
                {
                    if( uxNewPriority >= pxCurrentTCB->uxPriority )
                    {
                        xYieldRequired = pdTRUE;
                    }
                }
            }
            else if( pxTCB == pxCurrentTCB )
            {
                xYieldRequired = pdTRUE;
            }

            uxPriorityUsedOnEntry = pxTCB->uxPriority;

            #if ( configUSE_MUTEXES == 1 )
            {
                if( pxTCB->uxBasePriority == pxTCB->uxPriority )
                {
                    pxTCB->uxPriority = uxNewPriority;
                }
                pxTCB->uxBasePriority = uxNewPriority;
            }
            #else
            {
                pxTCB->uxPriority = uxNewPriority;
            }
            #endif

            listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ),
                                     ( ( TickType_t ) configMAX_PRIORITIES -
                                       ( TickType_t ) uxNewPriority ) );

            if( listIS_CONTAINED_WITHIN(
                    &( pxReadyTasksLists[ uxPriorityUsedOnEntry ] ),
                    &( pxTCB->xStateListItem ) ) != pdFALSE )
            {
                if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                {
                    taskRESET_READY_PRIORITY( uxPriorityUsedOnEntry );
                }

                prvAddTaskToReadyList( pxTCB );
            }

            if( xYieldRequired != pdFALSE )
            {
                taskYIELD_IF_USING_PREEMPTION();
            }

            configASSERT( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) ==
                            ( ( TickType_t ) configMAX_PRIORITIES -
                              ( TickType_t ) pxTCB->uxPriority ) ) );
        }
    }
    taskEXIT_CRITICAL();
}

#endif /* INCLUDE_vTaskPrioritySet */
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_xTaskGetCurrentTaskHandle == 1 )

TaskHandle_t xTaskGetCurrentTaskHandle( void )
{
    TaskHandle_t xReturn;

    xReturn = pxCurrentTCB;
    return xReturn;
}

#endif
/* ----------------------------------------------------------------------- */

#if ( INCLUDE_eTaskGetState == 1 )

eTaskState eTaskGetState( TaskHandle_t xTask )
{
    eTaskState eReturn;
    List_t const * pxStateList, * pxDelayedList, * pxOverflowedDelayedList;
    const TCB_t * const pxTCB = ( const TCB_t * ) xTask;

    configASSERT( pxTCB );

    if( pxTCB == pxCurrentTCB )
    {
        eReturn = eRunning;
    }
    else
    {
        taskENTER_CRITICAL();
        {
            pxStateList            = listLIST_ITEM_CONTAINER( &( pxTCB->xStateListItem ) );
            pxDelayedList          = pxDelayedTaskList;
            pxOverflowedDelayedList = pxOverflowDelayedTaskList;
        }
        taskEXIT_CRITICAL();

        if( ( pxStateList == pxDelayedList ) ||
            ( pxStateList == pxOverflowedDelayedList ) )
        {
            eReturn = eBlocked;
        }

        #if ( INCLUDE_vTaskSuspend == 1 )
        else if( pxStateList == &xSuspendedTaskList )
        {
            if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL )
            {
                eReturn = eSuspended;
            }
            else
            {
                eReturn = eBlocked;
            }
        }
        #endif

        #if ( INCLUDE_vTaskDelete == 1 )
        else if( ( pxStateList == &xTasksWaitingTermination ) ||
                 ( pxStateList == NULL ) )
        {
            eReturn = eDeleted;
        }
        #endif

        else
        {
            eReturn = eReady;
        }
    }

    return eReturn;
}

#endif /* INCLUDE_eTaskGetState */
/* ----------------------------------------------------------------------- */

#if ( configUSE_MUTEXES == 1 )

TaskHandle_t pvTaskIncrementMutexHeldCount( void )
{
    if( pxCurrentTCB != NULL )
    {
        ( pxCurrentTCB->uxMutexesHeld )++;
    }
    return pxCurrentTCB;
}

#endif /* configUSE_MUTEXES */
/* ----------------------------------------------------------------------- */

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify,
                                UBaseType_t uxIndexToNotify,
                                uint32_t ulValue,
                                eNotifyAction eAction,
                                uint32_t * pulPreviousNotificationValue )
{
    TCB_t * pxTCB;
    BaseType_t xReturn = pdPASS;
    uint8_t ucOriginalNotifyState;

    configASSERT( uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES );
    configASSERT( xTaskToNotify );
    pxTCB = ( TCB_t * ) xTaskToNotify;

    taskENTER_CRITICAL();
    {
        if( pulPreviousNotificationValue != NULL )
        {
            *pulPreviousNotificationValue = pxTCB->ulNotifiedValue[ uxIndexToNotify ];
        }

        ucOriginalNotifyState = pxTCB->ucNotifyState[ uxIndexToNotify ];
        pxTCB->ucNotifyState[ uxIndexToNotify ] = taskNOTIFICATION_RECEIVED;

        switch( eAction )
        {
            case eSetBits:
                pxTCB->ulNotifiedValue[ uxIndexToNotify ] |= ulValue;
                break;
            case eIncrement:
                pxTCB->ulNotifiedValue[ uxIndexToNotify ]++;
                break;
            case eSetValueWithOverwrite:
                pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                break;
            case eSetValueWithoutOverwrite:
                if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
                {
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                }
                else
                {
                    xReturn = pdFAIL;
                }
                break;
            case eNoAction:
                break;
            default:
                configASSERT( pdFALSE );
                break;
        }

        if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
        {
            ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
            prvAddTaskToReadyList( pxTCB );

            if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
            {
                taskYIELD_IF_USING_PREEMPTION();
            }
        }
    }
    taskEXIT_CRITICAL();

    return xReturn;
}

BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify,
                                       UBaseType_t uxIndexToNotify,
                                       uint32_t ulValue,
                                       eNotifyAction eAction,
                                       uint32_t * pulPreviousNotificationValue,
                                       BaseType_t * pxHigherPriorityTaskWoken )
{
    TCB_t * pxTCB;
    uint8_t ucOriginalNotifyState;
    BaseType_t xReturn = pdPASS;
    UBaseType_t uxSavedInterruptStatus;

    configASSERT( xTaskToNotify );
    configASSERT( uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES );

    pxTCB = ( TCB_t * ) xTaskToNotify;

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
    {
        if( pulPreviousNotificationValue != NULL )
        {
            *pulPreviousNotificationValue = pxTCB->ulNotifiedValue[ uxIndexToNotify ];
        }

        ucOriginalNotifyState = pxTCB->ucNotifyState[ uxIndexToNotify ];
        pxTCB->ucNotifyState[ uxIndexToNotify ] = taskNOTIFICATION_RECEIVED;

        switch( eAction )
        {
            case eSetBits:
                pxTCB->ulNotifiedValue[ uxIndexToNotify ] |= ulValue;
                break;
            case eIncrement:
                ( pxTCB->ulNotifiedValue[ uxIndexToNotify ] )++;
                break;
            case eSetValueWithOverwrite:
                pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                break;
            case eSetValueWithoutOverwrite:
                if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
                {
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                }
                else
                {
                    xReturn = pdFAIL;
                }
                break;
            case eNoAction:
                break;
            default:
                configASSERT( pdFALSE );
                break;
        }

        if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
        {
            ( void ) uxListRemove( &( pxTCB->xStateListItem ) );

            if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
            {
                prvAddTaskToReadyList( pxTCB );
            }
            else
            {
                vListInsertEnd( &xPendingReadyList, &( pxTCB->xEventListItem ) );
            }

            if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
            {
                if( pxHigherPriorityTaskWoken != NULL )
                {
                    *pxHigherPriorityTaskWoken = pdTRUE;
                }
                xYieldPending = pdTRUE;
            }
        }
    }
    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

    return xReturn;
}

void vTaskGenericNotifyGiveFromISR( TaskHandle_t xTaskToNotify,
                                     UBaseType_t uxIndexToNotify,
                                     BaseType_t * pxHigherPriorityTaskWoken )
{
    ( void ) xTaskGenericNotifyFromISR( xTaskToNotify, uxIndexToNotify,
                                        0, eIncrement, NULL,
                                        pxHigherPriorityTaskWoken );
}

BaseType_t xTaskGenericNotifyWait( UBaseType_t uxIndexToWaitOn,
                                    uint32_t ulBitsToClearOnEntry,
                                    uint32_t ulBitsToClearOnExit,
                                    uint32_t * pulNotificationValue,
                                    TickType_t xTicksToWait )
{
    BaseType_t xReturn;

    configASSERT( uxIndexToWaitOn < configTASK_NOTIFICATION_ARRAY_ENTRIES );

    taskENTER_CRITICAL();
    {
        if( pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] != taskNOTIFICATION_RECEIVED )
        {
            pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] &= ~ulBitsToClearOnEntry;
            pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskWAITING_NOTIFICATION;

            if( xTicksToWait > ( TickType_t ) 0 )
            {
                prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
                portYIELD_WITHIN_API();
            }
        }
    }
    taskEXIT_CRITICAL();

    taskENTER_CRITICAL();
    {
        if( pulNotificationValue != NULL )
        {
            *pulNotificationValue = pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ];
        }

        if( pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] != taskNOTIFICATION_RECEIVED )
        {
            xReturn = pdFALSE;
        }
        else
        {
            pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] &= ~ulBitsToClearOnExit;
            xReturn = pdTRUE;
        }

        pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskNOT_WAITING_NOTIFICATION;
    }
    taskEXIT_CRITICAL();

    return xReturn;
}

uint32_t ulTaskGenericNotifyTake( UBaseType_t uxIndexToWaitOn,
                                   BaseType_t xClearCountOnExit,
                                   TickType_t xTicksToWait )
{
    uint32_t ulReturn;

    configASSERT( uxIndexToWaitOn < configTASK_NOTIFICATION_ARRAY_ENTRIES );

    taskENTER_CRITICAL();
    {
        if( pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] == 0UL )
        {
            pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskWAITING_NOTIFICATION;

            if( xTicksToWait > ( TickType_t ) 0 )
            {
                prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
                portYIELD_WITHIN_API();
            }
        }
    }
    taskEXIT_CRITICAL();

    taskENTER_CRITICAL();
    {
        ulReturn = pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ];

        if( ulReturn != 0UL )
        {
            if( xClearCountOnExit != pdFALSE )
            {
                pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] = 0UL;
            }
            else
            {
                pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] = ulReturn - ( uint32_t ) 1;
            }
        }

        pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskNOT_WAITING_NOTIFICATION;
    }
    taskEXIT_CRITICAL();

    return ulReturn;
}

BaseType_t xTaskGenericNotifyStateClear( TaskHandle_t xTask,
                                          UBaseType_t uxIndexToClear )
{
    TCB_t * pxTCB;
    BaseType_t xReturn;

    configASSERT( uxIndexToClear < configTASK_NOTIFICATION_ARRAY_ENTRIES );

    pxTCB = ( xTask == NULL ) ? pxCurrentTCB : ( TCB_t * ) xTask;

    taskENTER_CRITICAL();
    {
        if( pxTCB->ucNotifyState[ uxIndexToClear ] == taskNOTIFICATION_RECEIVED )
        {
            pxTCB->ucNotifyState[ uxIndexToClear ] = taskNOT_WAITING_NOTIFICATION;
            xReturn = pdPASS;
        }
        else
        {
            xReturn = pdFAIL;
        }
    }
    taskEXIT_CRITICAL();

    return xReturn;
}

uint32_t ulTaskGenericNotifyValueClear( TaskHandle_t xTask,
                                         UBaseType_t uxIndexToClear,
                                         uint32_t ulBitsToClear )
{
    TCB_t * pxTCB;
    uint32_t ulReturn;

    pxTCB = ( xTask == NULL ) ? pxCurrentTCB : ( TCB_t * ) xTask;

    taskENTER_CRITICAL();
    {
        ulReturn = pxTCB->ulNotifiedValue[ uxIndexToClear ];
        pxTCB->ulNotifiedValue[ uxIndexToClear ] &= ~ulBitsToClear;
    }
    taskEXIT_CRITICAL();

    return ulReturn;
}

#endif /* configUSE_TASK_NOTIFICATIONS */
/* ----------------------------------------------------------------------- */

#if ( configUSE_TRACE_FACILITY == 1 )

UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                   const UBaseType_t uxArraySize,
                                   uint32_t * const pulTotalRunTime )
{
    /* Minimal stub – not used in this project */
    ( void ) pxTaskStatusArray;
    ( void ) uxArraySize;
    if( pulTotalRunTime != NULL )
    {
        *pulTotalRunTime = 0;
    }
    return 0;
}

void vTaskSetTaskNumber( TaskHandle_t xTask, const UBaseType_t uxHandle )
{
    TCB_t * pxTCB = ( TCB_t * ) xTask;
    if( pxTCB != NULL )
    {
        pxTCB->uxTaskNumber = uxHandle;
    }
}

UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask )
{
    const TCB_t * pxTCB = ( const TCB_t * ) xTask;
    if( pxTCB != NULL )
    {
        return pxTCB->uxTaskNumber;
    }
    return 0;
}

#endif /* configUSE_TRACE_FACILITY */
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait,
                                             const BaseType_t xCanBlockIndefinitely )
{
    TickType_t xTimeToWake;
    const TickType_t xConstTickCount = xTickCount;

    #if ( INCLUDE_vTaskDelete == 1 )
    {
        if( pxCurrentTCB->ucDeleted != 0 )
        {
            return;
        }
    }
    #endif

    /* Remove the task from the ready list before adding it to the blocked
     * list as the same list item is used for both lists. */
    if( uxListRemove( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
    {
        /* The current task must be in a ready list, so there is no need to
         * check, and the port reset macro can be called directly. */
        portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority, uxTopReadyPriority );
    }

    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        if( ( xTicksToWait == portMAX_DELAY ) && ( xCanBlockIndefinitely != pdFALSE ) )
        {
            /* Add the task to the suspended task list instead of a delayed
             * task list to ensure it is not woken by a timing event. */
            vListInsertEnd( &xSuspendedTaskList, &( pxCurrentTCB->xStateListItem ) );
            return;
        }
    }
    #endif

    /* Calculate the time at which the task should be woken if the event
     * does not occur.  This may overflow but this doesn't matter, the
     * kernel will manage it correctly. */
    xTimeToWake = xConstTickCount + xTicksToWait;

    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );

    if( xTimeToWake < xConstTickCount )
    {
        /* Wake time has overflowed.  Place this item in the overflow list. */
        vListInsert( pxOverflowDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
    }
    else
    {
        /* The wake time has not overflowed, so the current block list is
         * used. */
        vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );

        /* If the task entering the blocked state was placed at the head of
         * the list of blocked tasks then xNextTaskUnblockTime needs to be
         * updated too. */
        if( xTimeToWake < xNextTaskUnblockTime )
        {
            xNextTaskUnblockTime = xTimeToWake;
        }
    }
}

#ifndef portRESET_READY_PRIORITY
    #define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority ) \
    {                                                                   \
        if( ( uxPriority ) == ( uxTopReadyPriority ) )                 \
        {                                                               \
            ( uxTopReadyPriority )--;                                   \
        }                                                               \
    }
#endif

/* ----------------------------------------------------------------------- */

/* Dummy application hook implementations (weak, can be overridden) */
__attribute__(( weak )) void vApplicationIdleHook( void ) {}
__attribute__(( weak )) void vApplicationTickHook( void ) {}
__attribute__(( weak )) void vApplicationMallocFailedHook( void ) {}
__attribute__(( weak )) void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcName )
{
    ( void ) xTask;
    ( void ) pcName;
    for( ; ; ) {}
}
