// мінімальний stm32f4xx_hal_conf.h для hw-18
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED

#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED

#if !defined(HSE_VALUE)
#define HSE_VALUE 8000000U
#endif
#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT 100U
#endif
#if !defined(HSI_VALUE)
#define HSI_VALUE 16000000U
#endif
#if !defined(LSI_VALUE)
#define LSI_VALUE 32000U
#endif
#if !defined(LSE_VALUE)
#define LSE_VALUE 32768U
#endif
#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT 5000U
#endif
#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE 12288000U
#endif

#define VDD_VALUE 3300U
#define TICK_INT_PRIORITY 0x0FU
#define USE_RTOS 0U
#define PREFETCH_ENABLE 1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE 1U
#define USE_SPI_CRC 1U

/* перевизначаємо weak-функції напряму, як у лекції. */
#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_SPI_REGISTER_CALLBACKS 0U
#define USE_HAL_TIM_REGISTER_CALLBACKS 0U

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f4xx_hal_rcc.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f4xx_hal_flash.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f4xx_hal_gpio.h"
#endif
#ifdef HAL_EXTI_MODULE_ENABLED
#include "stm32f4xx_hal_exti.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32f4xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f4xx_hal_cortex.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32f4xx_hal_pwr.h"
#endif
#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32f4xx_hal_spi.h"
#endif
#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32f4xx_hal_tim.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f4xx_hal_uart.h"
#endif
#ifdef HAL_IWDG_MODULE_ENABLED
#include "stm32f4xx_hal_iwdg.h"
#endif

#define assert_param(expr) ((void)0U)

#ifdef __cplusplus
}
#endif
