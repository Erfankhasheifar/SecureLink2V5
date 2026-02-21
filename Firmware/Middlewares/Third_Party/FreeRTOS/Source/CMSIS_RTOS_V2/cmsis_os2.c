/**
 * @file cmsis_os2.c
 * @brief CMSIS-RTOS2 wrapper for FreeRTOS used by the FIFO_Bridge project.
 *
 * This file provides the minimal CMSIS-RTOS2 API implementation required by
 * main.c and fifo_bridge.c:
 *
 *   osKernelInitialize() – prepares the kernel before tasks are created.
 *   osKernelStart()      – hands control to the FreeRTOS scheduler.
 *   osThreadNew()        – creates a FreeRTOS task and returns its handle.
 *   osThreadYield()      – yields the CPU to the next ready task.
 *
 * Priority mapping
 * ----------------
 * CMSIS-RTOS2 priorities run from osPriorityIdle (1) to osPriorityISR (56).
 * FreeRTOS priorities run from tskIDLE_PRIORITY (0) upward.
 * Mapping: freertos_prio = cmsis_prio - osPriorityIdle
 *
 * Place this file in:
 *   Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
 * and add it to the STM32CubeIDE project build.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* ======================================================================== */
osStatus_t osKernelInitialize(void)
{
    /* FreeRTOS does not require explicit kernel initialization before tasks
       are created; the data structures are initialized lazily by xTaskCreate.
       Return success to satisfy the CMSIS-RTOS2 API. */
    return osOK;
}

/* ======================================================================== */
osStatus_t osKernelStart(void)
{
    /* Hand control to the FreeRTOS scheduler – does not return on success. */
    vTaskStartScheduler();

    /* Reaching here means there was insufficient heap for the idle task. */
    return osError;
}

/* ======================================================================== */
osThreadId_t osThreadNew(osThreadFunc_t func,
                         void *argument,
                         const osThreadAttr_t *attr)
{
    TaskHandle_t handle = NULL;
    uint32_t     stack_depth;
    UBaseType_t  priority;
    const char  *name;

    if (attr != NULL)
    {
        /* Stack size in the attribute is in bytes; FreeRTOS expects words. */
        stack_depth = (attr->stack_size > 0U)
                      ? (attr->stack_size / sizeof(StackType_t))
                      : (uint32_t)configMINIMAL_STACK_SIZE;

        /* Map CMSIS-RTOS2 priority to FreeRTOS priority.
           osPriorityIdle == 1, tskIDLE_PRIORITY == 0, so subtract 1. */
        priority = (UBaseType_t)((int32_t)attr->priority - (int32_t)osPriorityIdle);
        if ((int32_t)priority < (int32_t)tskIDLE_PRIORITY)
        {
            priority = tskIDLE_PRIORITY;
        }
        if (priority >= (UBaseType_t)configMAX_PRIORITIES)
        {
            priority = (UBaseType_t)(configMAX_PRIORITIES - 1U);
        }

        name = (attr->name != NULL) ? attr->name : "Task";
    }
    else
    {
        stack_depth = (uint32_t)configMINIMAL_STACK_SIZE;
        priority    = tskIDLE_PRIORITY + 1U;
        name        = "Task";
    }

    if (xTaskCreate((TaskFunction_t)func,
                    name,
                    (configSTACK_DEPTH_TYPE)stack_depth,
                    argument,
                    priority,
                    &handle) != pdPASS)
    {
        return NULL;
    }

    return (osThreadId_t)handle;
}

/* ======================================================================== */
osStatus_t osThreadYield(void)
{
    taskYIELD();
    return osOK;
}
