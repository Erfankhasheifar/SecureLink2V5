/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef STACK_MACROS_H
#define STACK_MACROS_H

/*
 * Stack overflow check macros – only active when
 * configCHECK_FOR_STACK_OVERFLOW is set to 1 or 2.
 */

#if ( configCHECK_FOR_STACK_OVERFLOW == 0 )

    #define taskFIRST_CHECK_FOR_STACK_OVERFLOW()
    #define taskSECOND_CHECK_FOR_STACK_OVERFLOW()

#endif /* configCHECK_FOR_STACK_OVERFLOW == 0 */

/* ---- Method 1: check if the stack pointer is within bounds ------------- */
#if ( ( configCHECK_FOR_STACK_OVERFLOW == 1 ) && ( portSTACK_GROWTH < 0 ) )

    #define taskFIRST_CHECK_FOR_STACK_OVERFLOW()                                                \
        do {                                                                                    \
            if( pxCurrentTCB->pxTopOfStack <= pxCurrentTCB->pxStack + 3 )                      \
            {                                                                                   \
                vApplicationStackOverflowHook( ( TaskHandle_t ) pxCurrentTCB,                  \
                                               pxCurrentTCB->pcTaskName );                     \
            }                                                                                   \
        } while( 0 )

    /* Second check not used with method 1 */
    #define taskSECOND_CHECK_FOR_STACK_OVERFLOW()

#endif /* configCHECK_FOR_STACK_OVERFLOW == 1 */

/* ---- Method 2: also check a fill pattern -------------------------------- */
#if ( ( configCHECK_FOR_STACK_OVERFLOW == 2 ) && ( portSTACK_GROWTH < 0 ) )

    #define taskFIRST_CHECK_FOR_STACK_OVERFLOW()                                                \
        do {                                                                                    \
            if( pxCurrentTCB->pxTopOfStack <= pxCurrentTCB->pxStack + 3 )                      \
            {                                                                                   \
                vApplicationStackOverflowHook( ( TaskHandle_t ) pxCurrentTCB,                  \
                                               pxCurrentTCB->pcTaskName );                     \
            }                                                                                   \
        } while( 0 )

    #define taskSECOND_CHECK_FOR_STACK_OVERFLOW()                                               \
        do {                                                                                    \
            static const uint8_t ucExpectedStackBytes[] = {                                    \
                tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,                   \
                tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,                   \
                tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,                   \
                tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,                   \
                tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,                   \
                tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,                   \
                tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE                                         \
            };                                                                                  \
            if( memcmp( pxCurrentTCB->pxStack, ucExpectedStackBytes,                           \
                        sizeof( ucExpectedStackBytes ) ) != 0 )                                 \
            {                                                                                   \
                vApplicationStackOverflowHook( ( TaskHandle_t ) pxCurrentTCB,                  \
                                               pxCurrentTCB->pcTaskName );                     \
            }                                                                                   \
        } while( 0 )

#endif /* configCHECK_FOR_STACK_OVERFLOW == 2 */

#endif /* STACK_MACROS_H */
