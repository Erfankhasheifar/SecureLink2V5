/**
 * @file fifo_bridge.c
 * @brief FreeRTOS ReaderTask and WriterTask for the FT2232HL 245-Sync-FIFO
 *        bridge on STM32H750 DevEBox.
 *
 * -------------------------------------------------------------------------
 * 245 Synchronous FIFO read protocol (FIFO#1, FT2232HL → MCU):
 *
 *   1. Assert OE# low  (MCU takes bus ownership)
 *   2. Wait until RXF# goes low (data available)
 *   3. Assert RD# low  (latch byte on next rising CLKOUT edge inside FT chip)
 *   4. Sample data bus after one CLKOUT period (≈16 ns @ 60 MHz)
 *   5. Deassert RD# high (one CLKOUT period between consecutive reads is fine)
 *   6. Repeat from step 2, or deassert OE# when done
 *
 * Because the STM32H750 runs at 480 MHz and CLKOUT is 60 MHz the MCU sees
 * each CLKOUT period as ~8 CPU cycles.  A small __NOP() delay after asserting
 * RD# is sufficient to satisfy the setup time without using a timer.
 *
 * 245 Synchronous FIFO write protocol (FIFO#2, MCU → FT2232HL):
 *
 *   1. Wait until TXE# goes low (transmit buffer not full)
 *   2. Drive data onto PF[7:0]
 *   3. Assert WR# low for ≥1 CLKOUT period
 *   4. Deassert WR# high
 *   5. Repeat from step 1
 *
 * -------------------------------------------------------------------------
 * Cache note (STM32H7):
 *   The ring buffer (g_bridge_buf) is in AXI-SRAM and is accessed only by the
 *   CPU.  The Cortex-M7 hardware manages coherency automatically.  No explicit
 *   cache maintenance (SCB_CleanDCache / SCB_InvalidateDCache) is required
 *   here.  A compiler memory barrier (__asm volatile("" ::: "memory")) in
 *   ring_buffer.h is sufficient to prevent the compiler from reordering
 *   loads/stores across the head/tail updates.
 * -------------------------------------------------------------------------
 */

#include "fifo_bridge.h"
#include "cmsis_os.h"

/* ---- Private helpers --------------------------------------------------- */

/** Tiny busy-wait: ~N * 2 CPU cycles at any optimisation level */
static inline void delay_cycles(uint32_t n)
{
    while (n--) {
        __asm volatile ("nop");
    }
}

/* ======================================================================== */
/**
 * @brief ReaderTask – reads bytes from FIFO#1 (FT2232HL Channel A, PC→MCU)
 *        and pushes them into the shared ring buffer.
 *
 * The task yields to the scheduler (osThreadYield) when either:
 *   - RXF# is not active (no data in the FT2232HL receive FIFO), or
 *   - The ring buffer is full (back-pressure from WriterTask).
 */
void StartReaderTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        /* Wait for data to be available in FIFO#1 */
        if (!FIFO1_RXF_ACTIVE() || rb_full(&g_bridge_buf))
        {
            osThreadYield();
            continue;
        }

        /* Assert OE# to enable FT2232HL output drivers */
        FIFO1_OE_ASSERT();
        delay_cycles(2); /* setup time: ≥1 CLKOUT period */

        /* Burst-read while data available and ring buffer has space */
        while (FIFO1_RXF_ACTIVE() && !rb_full(&g_bridge_buf))
        {
            /* Assert RD# – FT2232HL latches IDR on next rising CLKOUT */
            FIFO1_RD_ASSERT();
            delay_cycles(4); /* ≥1 CLKOUT period @ 60 MHz = ~8 CPU cycles */

            /* Sample data bus */
            uint8_t byte = FIFO1_READ_DATA();

            /* Deassert RD# */
            FIFO1_RD_DEASSERT();
            delay_cycles(2);

            /* Push to ring buffer (cannot fail: checked rb_full above) */
            rb_push(&g_bridge_buf, byte);
        }

        /* Deassert OE# to release bus */
        FIFO1_OE_DEASSERT();

        /* Yield to let WriterTask drain the buffer */
        osThreadYield();
    }
}

/* ======================================================================== */
/**
 * @brief WriterTask – pops bytes from the shared ring buffer and writes them
 *        to FIFO#2 (FT2232HL Channel A, MCU→PC).
 *
 * The task yields when either:
 *   - The ring buffer is empty (nothing to send), or
 *   - TXE# is not active (FIFO#2 transmit buffer is full).
 */
void StartWriterTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        /* Wait for data in ring buffer and space in FIFO#2 */
        if (rb_empty(&g_bridge_buf) || !FIFO2_TXE_ACTIVE())
        {
            osThreadYield();
            continue;
        }

        /* Burst-write while ring buffer has data and FIFO#2 can accept */
        while (!rb_empty(&g_bridge_buf) && FIFO2_TXE_ACTIVE())
        {
            uint8_t byte;
            rb_pop(&g_bridge_buf, &byte);

            /* Drive data bus */
            FIFO2_WRITE_DATA(byte);
            delay_cycles(2); /* data setup time */

            /* Pulse WR# low for ≥1 CLKOUT period */
            FIFO2_WR_ASSERT();
            delay_cycles(4);
            FIFO2_WR_DEASSERT();
            delay_cycles(2); /* WR# high time before next cycle */
        }

        osThreadYield();
    }
}
