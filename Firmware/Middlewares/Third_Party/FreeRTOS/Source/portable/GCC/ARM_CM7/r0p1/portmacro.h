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

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Type definitions -------------------------------------------------- */
#include <stdint.h>

#define portCHAR        char
#define portFLOAT       float
#define portDOUBLE      double
#define portLONG        long
#define portSHORT       short
#define portSTACK_TYPE  uint32_t
#define portBASE_TYPE   long

typedef portSTACK_TYPE   StackType_t;
typedef long             BaseType_t;
typedef unsigned long    UBaseType_t;

#if ( configUSE_16_BIT_TICKS == 1 )
    typedef uint16_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffff
#else
    typedef uint32_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffffffffUL
    #define portTICK_TYPE_IS_ATOMIC 1
#endif

/* ---- Architecture specifics -------------------------------------------- */
#define portSTACK_GROWTH        ( -1 )
#define portTICK_PERIOD_MS      ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT      8
#define portBYTE_ALIGNMENT_MASK ( portBYTE_ALIGNMENT - 1 )
#define portDONT_DISCARD        __attribute__(( used ))

/* ---- Scheduler utilities ----------------------------------------------- */
/* Trigger a PendSV exception to cause a context switch */
#define portNVIC_INT_CTRL_REG   ( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT  ( 1UL << 28UL )

#define portYIELD() \
    { \
        portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; \
        __asm volatile ( "dsb" ::: "memory" ); \
        __asm volatile ( "isb" ); \
    }

#define portEND_SWITCHING_ISR( xSwitchRequired ) \
    do { if( xSwitchRequired != pdFALSE ) portYIELD(); } while( 0 )

#define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )

/* ---- Critical section management --------------------------------------- */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portSET_INTERRUPT_MASK_FROM_ISR()       ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)   vPortSetBASEPRI(x)
#define portDISABLE_INTERRUPTS()                vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS()                 vPortSetBASEPRI(0)
#define portENTER_CRITICAL()                    vPortEnterCritical()
#define portEXIT_CRITICAL()                     vPortExitCritical()

/* ---- Tickless idle ----------------------------------------------------- */
#ifndef portSUPPRESS_TICKS_AND_SLEEP
    #define portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime )
#endif

/* ---- Task function macros ---------------------------------------------- */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) \
    void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) \
    void vFunction( void *pvParameters )

/* ---- Floating-point support (Cortex-M7 with FPU) ----------------------- */
/* The FPU context is saved lazily in the exception stack frame */
#define portFLOAT_ABI_HARD

/* ---- Memory barriers --------------------------------------------------- */
#define portMEMORY_BARRIER() __asm volatile( "" ::: "memory" )

/* ---- Inline BASEPRI manipulation --------------------------------------- */
static inline void vPortSetBASEPRI( uint32_t ulBASEPRI )
{
    __asm volatile
    (
        "   msr basepri, %0 \n"
        ::"r" ( ulBASEPRI ) : "memory"
    );
}

static inline void vPortRaiseBASEPRI( void )
{
    uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    __asm volatile
    (
        "   msr basepri, %0     \n"
        "   dsb                 \n"
        "   isb                 \n"
        ::"r" ( ulNewBASEPRI ) : "memory"
    );
}

static inline uint32_t ulPortRaiseBASEPRI( void )
{
    uint32_t ulOriginalBASEPRI, ulNewBASEPRI;
    ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    __asm volatile
    (
        "   mrs %0, basepri     \n"
        "   msr basepri, %1     \n"
        "   dsb                 \n"
        "   isb                 \n"
        :"=r" ( ulOriginalBASEPRI ) :"r" ( ulNewBASEPRI ) : "memory"
    );
    return ulOriginalBASEPRI;
}

#define portINLINE   __inline

/* ---- Assert ------------------------------------------------------------ */
#ifndef configASSERT
    #define configASSERT( x )
    #define configASSERT_DEFINED 0
#else
    #define configASSERT_DEFINED 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
