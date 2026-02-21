/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef PORTABLE_H
#define PORTABLE_H

/* Each port must provide this header at the path specified by portINCLUDE_PATH */
#ifdef FREERTOS_CONFIG_H
    #error "portable.h must be included before FreeRTOSConfig.h"
#endif

/* Include the port-specific macro definitions.
 * The Cortex-M7 portmacro.h is found via the include path set in the IDE
 * (Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1). */
#include "portmacro.h"

#include "mpu_wrappers.h"

/* Kernel provided function prototypes */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack,
                                    TaskFunction_t pxCode,
                                    void *pvParameters );

BaseType_t xPortStartScheduler( void );
void vPortEndScheduler( void );

#endif /* PORTABLE_H */
