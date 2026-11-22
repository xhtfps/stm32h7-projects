#include "main.h"
#include "app.h"

#define H7_TEMPLATE_HSE_VALUE       25000000UL
#define H7_TEMPLATE_SDRAM_BASE      0xC0000000UL

static void MPU_Config(void);
static void SystemClock_Config(void);

int main(void)
{
    MPU_Config();
    SCB_EnableICache();
    SCB_EnableDCache();

    HAL_Init();
    SystemClock_Config();
    App_Init();

    while (1)
    {
        App_Loop();
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    RCC_PeriphCLKInitTypeDef periph = {0};

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
    {
    }

    __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 5;
    osc.PLL.PLLN = 192;
    osc.PLL.PLLP = 2;
    osc.PLL.PLLQ = 2;
    osc.PLL.PLLR = 2;
    osc.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    osc.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                    RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clk.AHBCLKDivider = RCC_HCLK_DIV2;
    clk.APB3CLKDivider = RCC_APB3_DIV2;
    clk.APB1CLKDivider = RCC_APB1_DIV2;
    clk.APB2CLKDivider = RCC_APB2_DIV2;
    clk.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }

    periph.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_FMC;
#if H7_TEMPLATE_USE_LTDC_CLOCK
    periph.PeriphClockSelection |= RCC_PERIPHCLK_LTDC;
    periph.PLL3.PLL3M = 25;
    periph.PLL3.PLL3N = 330;
    periph.PLL3.PLL3P = 2;
    periph.PLL3.PLL3Q = 2;
    periph.PLL3.PLL3R = 10;
    periph.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
    periph.PLL3.PLL3VCOSEL = RCC_PLL3VCOMEDIUM;
    periph.PLL3.PLL3FRACN = 0;
#endif
    periph.FmcClockSelection = RCC_FMCCLKSOURCE_D1HCLK;
    periph.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&periph) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MPU_Config(void)
{
#if H7_TEMPLATE_USE_EXTERNAL_SDRAM
    MPU_Region_InitTypeDef region = {0};

    HAL_MPU_Disable();
    region.Enable = MPU_REGION_ENABLE;
    region.BaseAddress = H7_TEMPLATE_SDRAM_BASE;
    region.Size = MPU_REGION_SIZE_32MB;
    region.AccessPermission = MPU_REGION_FULL_ACCESS;
    region.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    region.IsCacheable = MPU_ACCESS_CACHEABLE;
    region.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    region.Number = MPU_REGION_NUMBER2;
    region.TypeExtField = MPU_TEX_LEVEL0;
    region.SubRegionDisable = 0x00U;
    region.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&region);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
#endif
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
