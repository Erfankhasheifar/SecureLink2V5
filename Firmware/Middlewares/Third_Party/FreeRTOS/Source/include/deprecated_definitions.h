/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef DEPRECATED_DEFINITIONS_H
#define DEPRECATED_DEFINITIONS_H

/* Map old names to new names to maintain backward compatibility */

#define eTaskStateGet               eTaskGetState
#define portTickType                TickType_t
#define xTaskHandle                 TaskHandle_t
#define xQueueHandle                QueueHandle_t
#define xSemaphoreHandle            SemaphoreHandle_t
#define xQueueSetHandle             QueueSetHandle_t
#define xQueueSetMemberHandle       QueueSetMemberHandle_t
#define xTimeOutType                TimeOut_t
#define xMemoryRegion               MemoryRegion_t
#define xTaskParameters             TaskParameters_t
#define xTaskStatusType             TaskStatus_t
#define xTimerHandle                TimerHandle_t
#define xCoRoutineHandle            CoRoutineHandle_t
#define pdTASK_HOOK_CODE            TaskHookFunction_t
#define portTICK_RATE_MS            portTICK_PERIOD_MS
#define pcTaskGetTaskName           pcTaskGetName
#define pcTimerGetTimerName         pcTimerGetName

#define portCRITICAL_NESTING_IN_TCB 0

#define tmrTIMER_CALLBACK           TimerCallbackFunction_t
#define pdTASK_CODE                 TaskFunction_t

#define xListItem                   ListItem_t
#define xList                       List_t

/* Backward compatible event group bit type */
#define xEventGroupHandle           EventGroupHandle_t

#endif /* DEPRECATED_DEFINITIONS_H */
