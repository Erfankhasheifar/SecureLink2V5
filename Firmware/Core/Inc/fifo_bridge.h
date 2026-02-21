/**
 * @file fifo_bridge.h
 * @brief GPIO register-level macros and task prototypes for the
 *        FT2232HL 245-Sync-FIFO bridge on STM32H750 DevEBox.
 *
 * Pin assignment (from problem statement):
 *
 *  FIFO#1 (PC Sender -> MCU)  –  "read" side
 *    Data bus (input)  : PE0..PE7
 *    RXF#  (input)     : PC0   – low = data available
 *    TXE#  (input)     : PC1   – low = transmit buffer NOT full
 *    RD#   (output)    : PC2   – pull low to clock a byte out
 *    WR#   (output)    : PC3   – not used in read-only direction (keep high)
 *    CLKOUT(input)     : PC4   – 60 MHz bus clock from FT2232HL
 *    OE#   (output)    : PC5   – pull low to enable output drivers
 *
 *  FIFO#2 (MCU -> PC Receiver) – "write" side
 *    Data bus (output) : PF0..PF7
 *    RXF#  (input)     : PD0   – optional: low = FT2232HL has data for us
 *    TXE#  (input)     : PD1   – low = transmit buffer NOT full (we can write)
 *    RD#   (output)    : PD2   – optional
 *    WR#   (output)    : PD3   – pull low then high to clock a byte in
 *    CLKOUT(input)     : PD4   – optional: 60 MHz clock
 *    OE#   (output)    : PD5   – optional
 */

#ifndef FIFO_BRIDGE_H
#define FIFO_BRIDGE_H

#include "stm32h7xx_hal.h"
#include "ring_buffer.h"

/* ---- Port/pin definitions ------------------------------------------ */

/* FIFO#1 data bus – input, PE0..PE7 */
#define FIFO1_DATA_PORT   GPIOE
#define FIFO1_DATA_MASK   0x00FFu

/* FIFO#1 control – GPIOC */
#define FIFO1_CTRL_PORT   GPIOC
#define FIFO1_RXF_PIN     GPIO_PIN_0   /* input  – active low */
#define FIFO1_TXE_PIN     GPIO_PIN_1   /* input  – active low */
#define FIFO1_RD_PIN      GPIO_PIN_2   /* output – active low */
#define FIFO1_WR_PIN      GPIO_PIN_3   /* output – active low (unused here) */
#define FIFO1_CLKOUT_PIN  GPIO_PIN_4   /* input  – 60 MHz */
#define FIFO1_OE_PIN      GPIO_PIN_5   /* output – active low */

/* FIFO#2 data bus – output, PF0..PF7 */
#define FIFO2_DATA_PORT   GPIOF
#define FIFO2_DATA_MASK   0x00FFu

/* FIFO#2 control – GPIOD */
#define FIFO2_CTRL_PORT   GPIOD
#define FIFO2_RXF_PIN     GPIO_PIN_0   /* input  – optional */
#define FIFO2_TXE_PIN     GPIO_PIN_1   /* input  – active low */
#define FIFO2_RD_PIN      GPIO_PIN_2   /* output – optional */
#define FIFO2_WR_PIN      GPIO_PIN_3   /* output – active low */
#define FIFO2_CLKOUT_PIN  GPIO_PIN_4   /* input  – optional */
#define FIFO2_OE_PIN      GPIO_PIN_5   /* output – optional */

/* ---- Direct-register helper macros --------------------------------- */

/** Read FIFO#1 data byte from PE[7:0] via IDR register */
#define FIFO1_READ_DATA()   ((uint8_t)(FIFO1_DATA_PORT->IDR & FIFO1_DATA_MASK))

/** Write byte to FIFO#2 via PF[7:0] BSRR (atomic set/reset) */
#define FIFO2_WRITE_DATA(b) \
    do { \
        FIFO2_DATA_PORT->BSRR = (uint32_t)(b) | \
                                 ((uint32_t)(~(b) & FIFO2_DATA_MASK) << 16); \
    } while (0)

/** Read FIFO#1 RXF# signal (active low: 0 = data ready) */
#define FIFO1_RXF_ACTIVE()  (!(FIFO1_CTRL_PORT->IDR & FIFO1_RXF_PIN))

/** Read FIFO#2 TXE# signal (active low: 0 = can write) */
#define FIFO2_TXE_ACTIVE()  (!(FIFO2_CTRL_PORT->IDR & FIFO2_TXE_PIN))

/** Assert FIFO#1 OE# (enable output drivers before read burst) */
#define FIFO1_OE_ASSERT()   (FIFO1_CTRL_PORT->BSRR = (uint32_t)FIFO1_OE_PIN  << 16)
#define FIFO1_OE_DEASSERT() (FIFO1_CTRL_PORT->BSRR = FIFO1_OE_PIN)

/** Assert / deassert FIFO#1 RD# */
#define FIFO1_RD_ASSERT()   (FIFO1_CTRL_PORT->BSRR = (uint32_t)FIFO1_RD_PIN  << 16)
#define FIFO1_RD_DEASSERT() (FIFO1_CTRL_PORT->BSRR = FIFO1_RD_PIN)

/** Assert / deassert FIFO#2 WR# */
#define FIFO2_WR_ASSERT()   (FIFO2_CTRL_PORT->BSRR = (uint32_t)FIFO2_WR_PIN  << 16)
#define FIFO2_WR_DEASSERT() (FIFO2_CTRL_PORT->BSRR = FIFO2_WR_PIN)

/* ---- Shared ring buffer -------------------------------------------- */
extern ring_buffer_t g_bridge_buf;

/* ---- FreeRTOS task prototypes -------------------------------------- */
void StartReaderTask(void *argument);
void StartWriterTask(void *argument);

#endif /* FIFO_BRIDGE_H */
