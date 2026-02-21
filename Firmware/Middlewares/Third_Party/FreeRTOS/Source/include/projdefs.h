/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef PROJ_DEFS_H
#define PROJ_DEFS_H

#include <stdint.h>

/* Boolean values */
#define pdFALSE    ( ( BaseType_t ) 0 )
#define pdTRUE     ( ( BaseType_t ) 1 )

#define pdPASS     ( pdTRUE )
#define pdFAIL     ( pdFALSE )

#define errQUEUE_EMPTY          ( ( BaseType_t ) 0 )
#define errQUEUE_FULL           ( ( BaseType_t ) 0 )

/* FreeRTOS error numbers */
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY   ( -1 )
#define errQUEUE_BLOCKED                        ( -4 )
#define errQUEUE_YIELD                          ( -5 )

/* Task function prototype macros (optional) */
#ifndef portTASK_FUNCTION_PROTO
    #define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) \
        void vFunction( void * pvParameters )
#endif

#ifndef portTASK_FUNCTION
    #define portTASK_FUNCTION( vFunction, pvParameters ) \
        void vFunction( void * pvParameters )
#endif

#endif /* PROJ_DEFS_H */
