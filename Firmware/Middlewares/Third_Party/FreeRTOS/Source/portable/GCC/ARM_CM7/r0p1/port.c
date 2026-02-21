/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 *
 * ARM Cortex-M7 port for FreeRTOS (GCC, hard-FP, r0p1 or later).
 */

/* Scheduler includes */
#include "FreeRTOS.h"
#include "task.h"

/* ---- Cortex-M7 SCB / SysTick registers --------------------------------- */
#define portNVIC_SYSTICK_CTRL_REG       ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG       ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG              ( *( ( volatile uint32_t * ) 0xe000ed20 ) )
#define portNVIC_SYSPRI2_REG            ( *( ( volatile uint32_t * ) 0xe000ed1c ) )
#define portNVIC_SYSTICK_CLK_BIT        ( 1UL << 2UL )
#define portNVIC_SYSTICK_INT_BIT        ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT     ( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT ( 1UL << 16UL )
#define portNVIC_PENDSV_PRI             ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI            ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )
#define portNVIC_INT_CTRL_REG           ( *( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT          ( 1UL << 28UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT ( 1UL << 25UL )
#define portNVIC_PEND_SYSTICK_SET_BIT   ( 1UL << 26UL )

#define portFPCCR                       ( *( ( volatile uint32_t * ) 0xe000ef34 ) )
#define portASPEN_AND_LSPEN_BITS        ( 0x3UL << 30UL )

/* Masks off all bits in the APSR other than the mode bits */
#define portAPSR_MODE_BITS_MASK         ( 0x1FUL )
/* The value of the mode bits in the APSR when the CPU is executing in user/
 * unprivileged mode. */
#define portAPSR_USER_MODE              ( 0x10UL )

/* Let the user override the default SysTick clock rate */
#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ      configCPU_CLOCK_HZ
    /* Ensure the SysTick is clocked at the same frequency as the core. */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG portNVIC_SYSTICK_CLK_BIT
#else
    /* Select the option to clock SysTick from the external reference clock. */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG ( 0 )
#endif

/* ---- Initial EXC_RETURN for task stack frame --------------------------- */
/* EXC_RETURN value to use when returning from an exception to Thread mode
 * using the PSP and with floating-point enabled (lazy stacking). */
#define portINITIAL_EXC_RETURN          ( 0xffffffedUL )

/* ---- Constants required to set up the initial stack -------------------- */
#define portINITIAL_XPSR                ( 0x01000000UL )

/* ---- Critical nesting counter ------------------------------------------ */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/* ---- Port initialise stack --------------------------------------------- */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /* Simulate the stack frame as it would be created by a context switch
     * interrupt.  See the book page 353 for information. */

    /* Offset added to account for the way the MCU uses the stack on entry/
     * exit of interrupts, and to ensure correct alignment. */
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_XPSR; /* xPSR */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode; /* PC */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS; /* LR */

    /* Save code space by skipping register initialisation. */
    pxTopOfStack -= 5; /* R12, R3, R2 and R1. */
    *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 */

    /* A save method is being used that requires each task to maintain its
     * own exec return value. */
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_EXC_RETURN;

    pxTopOfStack -= 8; /* R11, R10, R9, R8, R7, R6, R5 and R4. */

    return pxTopOfStack;
}

/* ---- Stack return address for task ------------------------------------- */
static void prvTaskExitError( void )
{
    volatile uint32_t ulDummy = 0;

    /* A function that implements a task must not exit or attempt to return to
     * its caller as there is nothing to return to.  If a task wants to exit it
     * should instead call vTaskDelete( NULL ). */
    configASSERT( ulDummy == 0 );

    /* Force an assert on the way out to make it clear something went wrong. */
    for( ; ; )
    {
        portNOP();
    }
}

#ifndef portTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif

/* ---- portNOP ----------------------------------------------------------- */
#ifndef portNOP
    #define portNOP() __asm volatile( "nop" )
#endif

/* ======================================================================== */
/* Context switch – Cortex-M7 with FPU (lazy stacking).
 * The PendSV handler is responsible for performing the context switch.
 * As PendSV has the lowest priority it runs last in an ISR nesting
 * situation, which is the correct time to perform a context switch.
 */
__attribute__( ( naked ) )
void xPortPendSVHandler( void )
{
    /* This is a naked function. */

    __asm volatile
    (
        "   mrs r0, psp                         \n"
        "   isb                                 \n"
        "                                       \n"
        "   ldr r3, pxCurrentTCBConst2          \n" /* Get the location of the current TCB. */
        "   ldr r2, [r3]                        \n"
        "                                       \n"
        "   tst r14, #0x10                      \n" /* Is the task using the FPU context?  If so, push high vfp registers. */
        "   it eq                               \n"
        "   vstmdbeq r0!, {s16-s31}             \n"
        "                                       \n"
        "   stmdb r0!, {r4-r11, r14}            \n" /* Save the core registers. */
        "   str r0, [r2]                        \n" /* Save the new top of stack into the first member of the TCB. */
        "                                       \n"
        "   stmdb sp!, {r0, r3}                 \n"
        "   mov r0, %0                          \n"
        "   msr basepri, r0                     \n"
        "   dsb                                 \n"
        "   isb                                 \n"
        "   bl vTaskSwitchContext               \n"
        "   mov r0, #0                          \n"
        "   msr basepri, r0                     \n"
        "   ldmia sp!, {r0, r3}                 \n"
        "                                       \n"
        "   ldr r1, [r3]                        \n" /* The first item in pxCurrentTCB is the task top of stack. */
        "   ldr r0, [r1]                        \n"
        "                                       \n"
        "   ldmia r0!, {r4-r11, r14}            \n" /* Pop the core registers. */
        "                                       \n"
        "   tst r14, #0x10                      \n" /* Is the task using the FPU context?  If so, pop the high vfp registers too. */
        "   it eq                               \n"
        "   vldmiaeq r0!, {s16-s31}             \n"
        "                                       \n"
        "   msr psp, r0                         \n"
        "   isb                                 \n"
        "                                       \n"
        "   bx r14                              \n"
        "                                       \n"
        "   .align 4                            \n"
        "pxCurrentTCBConst2: .word pxCurrentTCB \n"
        ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY )
    );
}
/* ----------------------------------------------------------------------- */

__attribute__( ( naked ) )
void vPortSVCHandler( void )
{
    __asm volatile
    (
        "   ldr r3, pxCurrentTCBConst           \n" /* Restore the context. */
        "   ldr r1, [r3]                        \n" /* Use pxCurrentTCBConst to get the pxCurrentTCB address. */
        "   ldr r0, [r1]                        \n" /* The first item in pxCurrentTCB is the task top of stack. */
        "   ldmia r0!, {r4-r11, r14}            \n" /* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
        "   msr psp, r0                         \n" /* Restore the task stack pointer. */
        "   isb                                 \n"
        "   mov r0, #0                          \n"
        "   msr basepri, r0                     \n"
        "   bx r14                              \n"
        "                                       \n"
        "   .align 4                            \n"
        "pxCurrentTCBConst: .word pxCurrentTCB  \n"
    );
}
/* ----------------------------------------------------------------------- */

static void prvPortStartFirstTask( void )
{
    /* Start the first task.  This also clears the bit that indicates the FPU
     * context should be saved. */
    __asm volatile
    (
        " ldr r0, =0xE000ED08   \n" /* Use the NVIC offset register to locate the stack. */
        " ldr r0, [r0]          \n"
        " ldr r0, [r0]          \n"
        " msr msp, r0           \n" /* Set the msp back to the start of the stack. */
        " mov r0, #0            \n" /* Clear the bit that indicates the FPU is in use, see comment above. */
        " msr control, r0       \n"
        " cpsie i               \n" /* Globally enable interrupts. */
        " cpsie f               \n"
        " dsb                   \n"
        " isb                   \n"
        " svc 0                 \n" /* System call to start first task. */
        " nop                   \n"
        " .ltorg                \n"
    );
}
/* ----------------------------------------------------------------------- */

/* ---- Forward declarations ---------------------------------------------- */
static void prvSetupTimerInterrupt( void );

/* ======================================================================== */
BaseType_t xPortStartScheduler( void )
{
    /* configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to 0.
     * See https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
    configASSERT( configMAX_SYSCALL_INTERRUPT_PRIORITY );

    /* Make PendSV and SysTick the lowest priority exceptions */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;

    /* Start the timer that generates the tick ISR.  Interrupts are disabled
     * here already. */
    prvSetupTimerInterrupt();

    /* Initialise the critical nesting count ready for the first task. */
    uxCriticalNesting = 0;

    /* Ensure the VFP is enabled – it should be but it's best not to rely on
     * CubeMX doing it. */
    portFPCCR |= portASPEN_AND_LSPEN_BITS;

    /* Start the first task. */
    prvPortStartFirstTask();

    /* Should never get here as the tasks will now be executing!  Call the task
     * exit error function to prevent compiler warnings about a static function
     * not being called in the case that the application writer overrides this
     * functionality by defining configTASK_RETURN_ADDRESS.  Call
     * vTaskSwitchContext() so link time optimisation does not remove the
     * symbol. */
    vTaskSwitchContext();

    /* Should not get here! */
    return 0;
}
/* ----------------------------------------------------------------------- */

void vPortEndScheduler( void )
{
    /* Not implemented in port as there is nothing to return to.
     * Artificially force an assert. */
    configASSERT( uxCriticalNesting == 1000UL );
}
/* ----------------------------------------------------------------------- */

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;

    /* This is not the interrupt safe version of the enter critical function so
     * assert() if it is being called from an interrupt context. */
    if( uxCriticalNesting == 1 )
    {
        configASSERT( ( portNVIC_INT_CTRL_REG & 0xff ) == 0 );
    }
}
/* ----------------------------------------------------------------------- */

void vPortExitCritical( void )
{
    configASSERT( uxCriticalNesting );
    uxCriticalNesting--;
    if( uxCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS();
    }
}
/* ----------------------------------------------------------------------- */

void xPortSysTickHandler( void )
{
    /* The SysTick runs at the lowest interrupt priority, so when this interrupt
     * executes all interrupts must be unmasked.  There is therefore no need to
     * save and then restore the interrupt mask value as its value is already
     * known – therefore the slightly optimised implementation shown below is
     * used instead of the standard critical section. */
    portDISABLE_INTERRUPTS();
    {
        /* Increment the RTOS tick. */
        if( xTaskIncrementTick() != pdFALSE )
        {
            /* A context switch is required.  Context switching is performed in
             * the PendSV interrupt.  Pend the PendSV interrupt. */
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
    }
    portENABLE_INTERRUPTS();
}
/* ----------------------------------------------------------------------- */

static void prvSetupTimerInterrupt( void )
{
    /* Calculate the constants required to configure the tick interrupt. */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CURRENT_REG = 0UL;

    /* Enable SysTick with the clock source and the interrupt */
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG |
                                   portNVIC_SYSTICK_INT_BIT        |
                                   portNVIC_SYSTICK_ENABLE_BIT );
}
