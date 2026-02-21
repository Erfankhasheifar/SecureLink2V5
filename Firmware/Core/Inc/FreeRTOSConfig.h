/*
 * FreeRTOS configuration for FIFO_Bridge on STM32H750 DevEBox.
 *
 * Clock: HSE 25 MHz -> PLL1 -> 480 MHz SYSCLK, AHB 240 MHz.
 * This file is included by FreeRTOS.h before any other FreeRTOS header.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Cortex-M7 CMSIS header is needed for __NVIC_PRIO_BITS */
#include "stm32h7xx.h"

/* ---- Scheduler --------------------------------------------------------- */
#define configUSE_PREEMPTION                    1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_TICKLESS_IDLE                 0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/* ---- Timing ------------------------------------------------------------ */
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 480000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )

/* ---- Task management --------------------------------------------------- */
#define configMAX_PRIORITIES                    ( 56 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 256 )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
#define configUSE_MINI_LIST_ITEM                1
#define configSTACK_DEPTH_TYPE                  uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
#define configHEAP_CLEAR_MEMORY_ON_FREE         0

/* ---- Memory ------------------------------------------------------------ */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) 16384 )

/* ---- Hook / trace ------------------------------------------------------ */
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_APPLICATION_TASK_TAG          0
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* ---- Co-routines ------------------------------------------------------- */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* ---- Software timers ---------------------------------------------------
 * Disabled: this project uses cooperative scheduling via osThreadYield and
 * does not require FreeRTOS software timers.
 * --------------------------------------------------------------------- */
#define configUSE_TIMERS                        0
#define configTIMER_TASK_PRIORITY               ( 2 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            256

/* ---- Event groups ------------------------------------------------------ */
#define configUSE_EVENT_GROUPS                  0

/* ---- Stream buffers ---------------------------------------------------- */
#define configUSE_STREAM_BUFFERS                0

/* ---- Optional API inclusion -------------------------------------------- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_uxTaskGetStackHighWaterMark2    0
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              1

/* ---- Cortex-M interrupt priority configuration ------------------------- */
/*
 * The Cortex-M7 inside STM32H750 has __NVIC_PRIO_BITS = 4 (16 levels).
 * configLIBRARY_LOWEST_INTERRUPT_PRIORITY must be the maximum priority
 * value (= 2^n - 1).
 * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY is the highest interrupt
 * priority from which FreeRTOS "FromISR" API may be called.
 */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

/* Convert library priority numbers to Cortex-M NVIC values (shift left) */
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Same as MAX_SYSCALL for backward compat */
#define configMAX_API_CALL_INTERRUPT_PRIORITY \
    configMAX_SYSCALL_INTERRUPT_PRIORITY

/* ---- Assert ------------------------------------------------------------ */
#define configASSERT( x ) \
    if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* ---- Map CMSIS-RTOS interrupt handlers to FreeRTOS handlers ------------ */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
