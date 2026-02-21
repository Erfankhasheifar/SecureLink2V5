/*
 * cmsis_os2.c – CMSIS-RTOS2 wrapper over FreeRTOS for FIFO_Bridge.
 *
 * Implements the four CMSIS-RTOS2 functions used by main.c and fifo_bridge.c:
 *   osKernelInitialize  →  (no-op; FreeRTOS initialises implicitly)
 *   osKernelStart       →  vTaskStartScheduler()
 *   osThreadNew         →  xTaskCreate()
 *   osThreadYield       →  taskYIELD()
 *
 * CMSIS-RTOS2 priority mapping (0..56) → FreeRTOS priority (0..configMAX_PRIORITIES-1):
 *   osPriorityNormal (24) → configMAX_PRIORITIES/2
 *   The offset is CMSIS_RTOS_PRIORITY_OFFSET (24 = osPriorityNormal).
 */

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* ---- Constants --------------------------------------------------------- */

/* The CMSIS-RTOS2 idle priority value. */
#define CMSIS_RTOS_PRIORITY_IDLE    ( 1 )

/* The CMSIS-RTOS2 "Normal" priority maps to a mid-level FreeRTOS priority.
 * CMSIS priorities run 1..56, osPriorityNormal == 24.
 * FreeRTOS priorities run 0..configMAX_PRIORITIES-1 (0 = idle).
 * Map: FreeRTOS_prio = cmsis_prio - CMSIS_RTOS_PRIORITY_IDLE + 1 */
static UBaseType_t prvCMSIS2FreeRTOSPriority( osPriority_t cmsis_prio )
{
    BaseType_t freertos_prio;

    if( cmsis_prio <= ( osPriority_t ) CMSIS_RTOS_PRIORITY_IDLE )
    {
        return tskIDLE_PRIORITY;
    }

    /* Linear mapping: shift so that osPriorityNormal gives a mid-level
     * FreeRTOS priority. */
    freertos_prio = ( BaseType_t ) cmsis_prio - ( BaseType_t ) CMSIS_RTOS_PRIORITY_IDLE;

    if( freertos_prio >= ( BaseType_t ) configMAX_PRIORITIES )
    {
        freertos_prio = ( BaseType_t ) configMAX_PRIORITIES - 1;
    }

    return ( UBaseType_t ) freertos_prio;
}

/* ======================================================================== */

/**
 * @brief  Initialise the RTOS kernel.
 *
 * FreeRTOS requires no explicit kernel initialisation – the kernel is set up
 * implicitly when the first task is created and the scheduler is started.
 * This function simply returns osOK to satisfy the CMSIS-RTOS2 contract.
 */
osStatus_t osKernelInitialize( void )
{
    /* Nothing to do for FreeRTOS – return success. */
    return osOK;
}

/* ----------------------------------------------------------------------- */

/**
 * @brief  Start the RTOS kernel scheduler.
 *
 * Calls vTaskStartScheduler().  On a successful start this function does not
 * return.  It returns osError only if the scheduler could not be started
 * (which typically indicates that there is not enough heap memory to create
 * the idle task).
 */
osStatus_t osKernelStart( void )
{
    vTaskStartScheduler();

    /* vTaskStartScheduler() should only return if there is insufficient heap
     * memory to create the idle task. */
    return osError;
}

/* ----------------------------------------------------------------------- */

/**
 * @brief  Create a thread and add it to active threads.
 *
 * Maps the CMSIS-RTOS2 osThreadAttr_t attributes to an xTaskCreate() call.
 * Supports dynamic allocation only (configSUPPORT_DYNAMIC_ALLOCATION must
 * be 1, which it is in this project's FreeRTOSConfig.h).
 */
osThreadId_t osThreadNew( osThreadFunc_t func,
                          void * argument,
                          const osThreadAttr_t * attr )
{
    TaskHandle_t task_handle = NULL;
    const char * task_name   = "";
    configSTACK_DEPTH_TYPE stack_depth = ( configSTACK_DEPTH_TYPE ) configMINIMAL_STACK_SIZE;
    UBaseType_t ux_priority  = tskIDLE_PRIORITY + 1U;

    if( func == NULL )
    {
        return NULL;
    }

    if( attr != NULL )
    {
        if( attr->name != NULL )
        {
            task_name = attr->name;
        }
        if( attr->stack_size > 0U )
        {
            /* attr->stack_size is in bytes; FreeRTOS uses words (4 bytes each) */
            stack_depth = ( configSTACK_DEPTH_TYPE ) ( attr->stack_size / sizeof( StackType_t ) );
        }
        if( attr->priority != osPriorityNone )
        {
            ux_priority = prvCMSIS2FreeRTOSPriority( attr->priority );
        }
    }

    if( xTaskCreate( ( TaskFunction_t ) func,
                     task_name,
                     stack_depth,
                     argument,
                     ux_priority,
                     &task_handle ) != pdPASS )
    {
        return NULL;
    }

    return ( osThreadId_t ) task_handle;
}

/* ----------------------------------------------------------------------- */

/**
 * @brief  Yield control to the next ready thread.
 *
 * Triggers a PendSV context-switch request via the portYIELD() macro (which
 * is called through the taskYIELD() wrapper).
 */
osStatus_t osThreadYield( void )
{
    taskYIELD();
    return osOK;
}
