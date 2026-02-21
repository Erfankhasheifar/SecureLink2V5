/**
 * @file main.c
 * @brief STM32H750 DevEBox – FT2232HL 245-Sync-FIFO bridge (main entry point).
 *
 * Clock configuration
 * -------------------
 *   External crystal : 25 MHz (HSE)
 *   PLL1             : VCO = 960 MHz  → SYSCLK = 480 MHz
 *   AHB              : /2  → 240 MHz
 *   APB1/APB2        : /2  → 120 MHz
 *
 * FreeRTOS tasks
 * --------------
 *   ReaderTask  – reads bytes from FIFO#1 (PE0..PE7) and pushes to ring buffer
 *   WriterTask  – pops bytes from ring buffer and writes to FIFO#2 (PF0..PF7)
 *
 * Cache note (STM32H7)
 * --------------------
 *   The Cortex-M7 D-cache is enabled by CubeMX in SystemInit().  The ring
 *   buffer lives in AXI-SRAM (D1 domain, starting at 0x24000000) which is
 *   covered by the default MPU region with Write-Back/Read-Allocate caching.
 *   Since only the CPU accesses the ring buffer (no DMA), cache coherency is
 *   maintained automatically by the hardware.  If you later add DMA, place the
 *   DMA buffers in a separate MPU region marked as Non-Cacheable.
 */

#include "main.h"
#include "cmsis_os.h"
#include "fifo_bridge.h"

/* ---- Shared ring buffer (producer: ReaderTask, consumer: WriterTask) --- */
ring_buffer_t g_bridge_buf;

/* ---- FreeRTOS thread attributes ---------------------------------------- */
const osThreadAttr_t readerTask_attributes = {
    .name       = "ReaderTask",
    .stack_size = 512 * 4,
    .priority   = (osPriority_t) osPriorityAboveNormal,
};

const osThreadAttr_t writerTask_attributes = {
    .name       = "WriterTask",
    .stack_size = 512 * 4,
    .priority   = (osPriority_t) osPriorityAboveNormal,
};

/* ---- Private function prototypes --------------------------------------- */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* ======================================================================== */
int main(void)
{
    /* HAL and cache initialisation (D/I-cache enabled in SystemInit) */
    HAL_Init();

    /* Configure system clock: HSE 25 MHz → 480 MHz SYSCLK */
    SystemClock_Config();

    /* Initialise GPIO peripherals */
    MX_GPIO_Init();

    /* Initialise ring buffer */
    rb_init(&g_bridge_buf);

    /* Initialise FreeRTOS kernel */
    osKernelInitialize();

    /* Create bridging tasks */
    osThreadNew(StartReaderTask, NULL, &readerTask_attributes);
    osThreadNew(StartWriterTask, NULL, &writerTask_attributes);

    /* Start scheduler – does not return */
    osKernelStart();

    /* Should never reach here */
    while (1) {}
}

/* ======================================================================== */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Supply configuration update enable */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /* Configure HSE and PLL1 */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    /* HSE=25 MHz, DIVM1=5 → 5 MHz ref, DIVN1=192 → 960 MHz VCO,
       DIVP1=2 → 480 MHz SYSCLK, DIVQ1=4 → 240 MHz, DIVR1=2 → 480 MHz */
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE  = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Select PLL as system clock; AHB /2, APBx /2 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK   | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2  |
                                   RCC_CLOCKTYPE_D3PCLK1| RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

/* ======================================================================== */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clocks */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();  /* OSC pins */

    /* ------------------------------------------------------------------
     * FIFO#1 data bus – PE0..PE7 : INPUT, no pull
     * ------------------------------------------------------------------ */
    GPIO_InitStruct.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                            GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* ------------------------------------------------------------------
     * FIFO#2 data bus – PF0..PF7 : OUTPUT PP, no pull, high-speed
     * ------------------------------------------------------------------ */
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                             GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* ------------------------------------------------------------------
     * FIFO#1 control (GPIOC):
     *   Inputs : PC0 (RXF#), PC1 (TXE#), PC4 (CLKOUT)
     *   Outputs: PC2 (RD#), PC3 (WR#), PC5 (OE#)
     * ------------------------------------------------------------------ */
    /* Inputs */
    GPIO_InitStruct.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Outputs – start de-asserted (high = inactive for active-low signals) */
    GPIO_InitStruct.Pin   = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIOC->BSRR = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5; /* RD#=1, WR#=1, OE#=1 */

    /* ------------------------------------------------------------------
     * FIFO#2 control (GPIOD):
     *   Inputs : PD0 (RXF# opt), PD1 (TXE#), PD4 (CLKOUT opt)
     *   Outputs: PD2 (RD# opt), PD3 (WR#), PD5 (OE# opt)
     * ------------------------------------------------------------------ */
    /* Inputs */
    GPIO_InitStruct.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Outputs – start de-asserted */
    GPIO_InitStruct.Pin   = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    GPIOD->BSRR = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5; /* RD#=1, WR#=1, OE#=1 */
}

/* ======================================================================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
