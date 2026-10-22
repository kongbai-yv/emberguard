# AGENTS

## Purpose
This repository is a small STM32 firmware project for an STM32F103-based sensor platform. It uses the ST Standard Peripheral Library and includes multiple BSP drivers for sensors, OLED display modules, and serial communication.

## Key entry points and directories
- `user/main.c`: firmware entry point and main application logic.
- `App/`: BSP and sensor driver source/header files, including DHT11, MQ-4, flame sensor, BMP180, MAX6675, OLED SPI, and HC-SR04.
- `my_lib/`: common hardware support libraries for delay, I2C, SPI, UART, and OLED display support.
- `std_periph_driver/`: ST Standard Peripheral Library source and headers.
- `startup/`: Cortex-M startup assembly files.
- `RTE/Device/STM32F103C8/`: runtime device selection and RTE component configuration.
- `template.uvprojx`, `template.uvguix.qwq`, `template.uvguix.ZhuanZ1`: Keil/UVision project templates present in the repository.

## Build and development notes
- There is no repository-level `Makefile` or CI build script.
- `.vscode/tasks.json` contains a local task that compiles the active file with MinGW `gcc.exe`; this is useful for quick syntax checks, but it is not the actual STM32 cross-compile workflow.
- The real firmware build is likely intended for Keil uVision / ARM toolchain using the provided `.uvprojx` and template files.

## Code conventions and constraints
- Code uses the STM32 Standard Peripheral Library API (`GPIO_InitTypeDef`, `USART_InitTypeDef`, `RCC_APB2PeriphClockCmd`, etc.).
- The firmware uses polling loops and low-level peripheral initialization rather than an RTOS.
- Comments and identifiers are often in Chinese; preserve intent and naming where practical.
- Avoid dynamic memory and heavy library dependencies. Keep changes simple and hardware-oriented.
- Preserve existing I/O and display update patterns in `user/main.c` when modifying behavior.

## What agents should do first
- Focus on `user/main.c` for application behavior and status update logic.
- Use `App/` and `my_lib/` drivers for peripheral support, not ad hoc new HAL wrappers.
- Do not assume an x86 build environment for final firmware output; prioritize STM32/Keil-style embedded code.

## Notes for future improvements
- If more project documentation is added, link it here instead of duplicating it.
- A future customization file could add automated checks for STM32 project structure or Keil template consistency.
