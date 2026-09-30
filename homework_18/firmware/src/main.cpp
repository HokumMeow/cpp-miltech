// STM32F4-Discovery
//   UART:  USART2 (PA2=TX, PA3=RX) -> USB-UART на платі
//   SPI:   SPI1 (PA5=SCK, PA6=MISO, PA7=MOSI, PE3=CS) -> вбудований акселерометр LIS3DSH
//   Таймер: TIM3 період виміру керується командою "p <ms>"
//   Кнопка: PA0 (EXTI0, rising) перемикає режим fast/slow
//   LED:    PD12 блимає в такт виміру

#include "app.hpp"
#include "stm32f4xx_hal.h"

namespace {

SPI_HandleTypeDef g_spi1;
UART_HandleTypeDef g_uart2;
TIM_HandleTypeDef g_tim3;
IWDG_HandleTypeDef g_iwdg;

volatile std::uint8_t g_uart_rx_byte = 0;

constexpr std::uint32_t kTimTickHz = 1000; 

void init_gpio() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  GPIO_InitTypeDef gpio{};

  // led pulse
  gpio.Pin = GPIO_PIN_12;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &gpio);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

    gpio.Pin = GPIO_PIN_3;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &gpio);
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);  // деселект за замовчуванням

  // PA5/PA6/PA7 - SPI1 SCK/MISO/MOSI
  gpio.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &gpio);

  // PA2/PA3 - USART2 TX/RX
  gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &gpio);

  // PA0 - кнопка 
  gpio.Pin = GPIO_PIN_0;
  gpio.Mode = GPIO_MODE_IT_RISING;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpio);
}

void init_spi1() {
  __HAL_RCC_SPI1_CLK_ENABLE();

  g_spi1.Instance = SPI1;
  g_spi1.Init.Mode = SPI_MODE_MASTER;
  g_spi1.Init.Direction = SPI_DIRECTION_2LINES;
  g_spi1.Init.DataSize = SPI_DATASIZE_8BIT;
  g_spi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  g_spi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  g_spi1.Init.NSS = SPI_NSS_SOFT;
  g_spi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  g_spi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  g_spi1.Init.TIMode = SPI_TIMODE_DISABLE;
  g_spi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  g_spi1.Init.CRCPolynomial = 7;
  HAL_SPI_Init(&g_spi1);
}

void init_usart2() {
  __HAL_RCC_USART2_CLK_ENABLE();

  g_uart2.Instance = USART2;
  g_uart2.Init.BaudRate = 115200;
  g_uart2.Init.WordLength = UART_WORDLENGTH_8B;
  g_uart2.Init.StopBits = UART_STOPBITS_1;
  g_uart2.Init.Parity = UART_PARITY_NONE;
  g_uart2.Init.Mode = UART_MODE_TX_RX;
  g_uart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  g_uart2.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&g_uart2);

  HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void init_tim3(std::uint32_t initial_period_ms) {
  __HAL_RCC_TIM3_CLK_ENABLE();

  g_tim3.Instance = TIM3;
  g_tim3.Init.Prescaler = (16000000U / kTimTickHz) - 1U;
  g_tim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  g_tim3.Init.Period = initial_period_ms - 1U;
  g_tim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  g_tim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_Base_Init(&g_tim3);

  HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

void init_button_irq() {
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  HAL_NVIC_SetPriority(EXTI0_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void init_iwdg() {
  // LSI ~32 кГц,
  // дільник /256 -> крок ~8 мс
  // Reload=250 -> тайм-аут ~2 c
  g_iwdg.Instance = IWDG;
  g_iwdg.Init.Prescaler = IWDG_PRESCALER_256;
  g_iwdg.Init.Reload = 250;
  HAL_IWDG_Init(&g_iwdg);
}

hw18::App g_app(&g_spi1, &g_uart2, &g_tim3, &g_iwdg, GPIOE, GPIO_PIN_3, GPIOD, GPIO_PIN_12);

}  // namespace

extern "C" {

void SysTick_Handler(void) { HAL_IncTick(); }

void EXTI0_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); }
void TIM3_IRQHandler(void) { HAL_TIM_IRQHandler(&g_tim3); }
void USART2_IRQHandler(void) { HAL_UART_IRQHandler(&g_uart2); }

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
  if (pin == GPIO_PIN_0) g_app.on_button_edge();  // лише прапорець - фільтр брязкоту контактів у циклі
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
  if (htim->Instance == TIM3) g_app.on_timer_tick();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
  if (huart->Instance == USART2) {
    g_app.on_uart_rx_byte(g_uart_rx_byte);
    HAL_UART_Receive_IT(&g_uart2, const_cast<uint8_t*>(&g_uart_rx_byte), 1);
  }
}

}  // extern "C"

int main(void) {
  HAL_Init();

  init_gpio();
  init_spi1();
  init_usart2();
  init_tim3(500);
  init_button_irq();
  init_iwdg();

  HAL_UART_Receive_IT(&g_uart2, const_cast<uint8_t*>(&g_uart_rx_byte), 1);
  HAL_TIM_Base_Start_IT(&g_tim3);

  g_app.begin();

  while (true) {
    g_app.poll();
  }
}
