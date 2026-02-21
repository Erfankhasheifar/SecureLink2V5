/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS configuration for the FIFO_Bridge project on STM32H750 DevEBox.
 *
 * Generated settings match the IOC file:
 *   FREERTOS.configMINIMAL_STACK_SIZE = 256
 *   FREERTOS.configTOTAL_HEAP_SIZE    = 16384
 *   VP_FREERTOS_VS_CMSIS_V2.Mode      = CMSIS_V2
 *
 * Cortex-M7 interrupt priority configuration (STM32H7 uses 4-bit grouping):
 *   configPRIO_BITS = 4  (__NVIC_PRIO_BITS on STM32H7)
 *   configLIBRARY_LOWEST_INTERRUPT_PRIORITY       = 15
 *   configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  =  5
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---- Scheduler --------------------------------------------------------- */
#define configUSE_PREEMPTION                    1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0
#define configCHECK_FOR_STACK_OVERFLOW          0

/* ---- Clock / tick ------------------------------------------------------ */
#define configCPU_CLOCK_HZ                      480000000UL
#define configTICK_RATE_HZ                      1000UL
#define configUSE_16_BIT_TICKS                  0

/* ---- Task sizes -------------------------------------------------------- */
#define configMINIMAL_STACK_SIZE                256U
#define configIDLE_SHOULD_YIELD                 1
#define configMAX_TASK_NAME_LEN                 16

/* ---- Priority levels --------------------------------------------------- */
/* Must cover full CMSIS-RTOS2 range (osPriorityIdle=1 … osPriorityISR=56). */
#define configMAX_PRIORITIES                    56

/* ---- Memory ------------------------------------------------------------ */
#define configTOTAL_HEAP_SIZE                   16384U

/* ---- Software timers --------------------------------------------------- */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ---- Optional features ------------------------------------------------- */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* ---- API inclusion ----------------------------------------------------- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              1

/* ---- Cortex-M7 interrupt priority -------------------------------------- */
/* STM32H7 implements 4-bit priority fields (__NVIC_PRIO_BITS = 4). */
#define configPRIO_BITS                         4

/* Lowest possible hardware interrupt priority (least urgent). */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY           15

/* Highest priority from which FreeRTOS API calls are permitted.
   Interrupts with a LOGICAL priority ABOVE this value (lower number)
   are not managed by FreeRTOS and must NOT call any API functions. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY       5

/* These map the above logical values into the NVIC register format. */
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* ---- Assertion handler ------------------------------------------------- */
#define configASSERT( x ) \
    do { if ( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for ( ;; ) {} } } while (0)

/* ---- FreeRTOS port-specific include ------------------------------------ */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
