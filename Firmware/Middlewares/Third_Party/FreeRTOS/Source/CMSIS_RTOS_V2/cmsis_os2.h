/*
 * CMSIS-RTOS2 API header (ARM standard).
 * FreeRTOS Kernel V10.3.1 – minimal subset for FIFO_Bridge project.
 *
 * Only the API elements referenced by main.c and fifo_bridge.c are
 * declared here.  The full specification is at:
 * https://arm-software.github.io/CMSIS_5/RTOS2/html/group__CMSIS__RTOS.html
 */

#ifndef CMSIS_OS2_H_
#define CMSIS_OS2_H_

/* Re-use the type definitions from the project-level cmsis_os.h so that
 * the two headers refer to the same types. */
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/* osStatus_t, osPriority_t, osThreadAttr_t, osThreadId_t, osThreadFunc_t,
 * osKernelInitialize, osKernelStart, osThreadNew, osThreadYield
 * are all defined / declared in cmsis_os.h already. */

#ifdef __cplusplus
}
#endif

#endif /* CMSIS_OS2_H_ */
