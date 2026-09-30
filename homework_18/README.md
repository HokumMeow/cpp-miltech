# ДЗ-18: пристрій на двох інтерфейсах - STM32F4-Discovery

Реалізація для реальної плати **STM32F4-Discovery **. Другий
інтерфейс (окрім UART) - **SPI**, а не I²C/АЦП/ШІМ з прикладів у завданні:
на платі вже впаяний акселерометр **LIS3DSH** (SPI1).

## Архітектура

```
homework_18/
  core/       - платформонезалежне ядро: формат звіту, розбір команд,
                кільцевий буфер приймання. Збирається і тестується на
                хості (GoogleTest), потім лінкується в прошивку без змін.
  firmware/   - STM32-специфічний шар: main.cpp (ініціалізація
                периферії HAL), app.cpp (оркестрація), lis3dsh.cpp
                (драйвер акселерометра).
```


## Розпіновка STM32F4-Discovery

| Сигнал | Пін | Периферія |
|---|---|---|
| UART TX | PA2 | USART2 |
| UART RX | PA3 | USART2 |
| Акселерометр SCK | PA5 | SPI1 |
| Акселерометр MISO | PA6 | SPI1 |
| Акселерометр MOSI | PA7 | SPI1 |
| Акселерометр CS | PE3 | GPIO (software NSS) |
| Кнопка USER | PA0 | EXTI0, rising edge |
| LED "пульс" | PD12 (зелений) | GPIO output |

**UART**: вбудований ST-LINK/V2 на цій платі (на відміну від Nucleo) не
має VCP-мосту, тому потрібен зовнішній USB-UART перехідник: TX плати
(PA2) → RX перехідника, RX плати (PA3) → TX перехідника, GND → GND.
115200 8N1.

**Акселерометр**: SPI1, CS керується вручну через PE3 (NSS software),
режим SPI 3 (CPOL=1, CPHA=1) - так, як задокументовано в датащиті
LIS3DSH


### Ядро - тести на хості

```bash
cmake -S . -B build -G Ninja        # з кореня репозиторію
cmake --build build --target hw18_core_tests
./build/homework_18/core/hw18_core_tests
```

### Прошивка - крос-компіляція arm-none-eabi

```bash
cd homework_18/firmware
cmake -S . -B build-firmware -G Ninja \
      --toolchain ../../cmake/toolchains/arm-none-eabi-stm32f4.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build-firmware
```

Результат: `build-firmware/hw18_firmware.elf` (+ `.bin`, `.map`)
