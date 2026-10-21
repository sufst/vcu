<h1 align="center"> VCU - Vehicle Control Unit </h2>

# About

The VCU is responsible for controlling the tractive system based on driver
input. The microcontroller used in this project is the STM32 F746ZG.

More detailed information about this project is available to members on the
SUFST Docs Site.

# Navigation

- [How to Test and Run the Firmware on a VCU (start here)](./docs/how-to-run.md)
- [Testing with Vector CANoe](./docs/canoe-testing.md)
- [Code Architecture Overview](./docs/code-overview.md)
- [ThreadX to FreeRTOS Port Notes](./docs/freertos-port.md)
- [Development Environment Setup](./docs/dev-env-setup.md)
- [GitHub Project-Specific Contributing Guidelines](./.github/CONTRIBUTING_EXTRA.md)

# Useful Resources

Microcontroller:

- [STM32F746ZG MCU Datasheet](https://www.st.com/resource/en/datasheet/stm32f746zg.pdf)
- [STM32F7xx HAL Manual](https://www.st.com/resource/en/user_manual/dm00189702-description-of-stm32f7-hal-and-lowlayer-drivers-stmicroelectronics.pdf)

FreeRTOS:

- [FreeRTOS Kernel Developer Docs](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/00-Developer-docs)
- [FreeRTOS API Reference](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/00-TaskHandle)
- [FreeRTOS on Cortex-M (interrupt priorities)](https://www.freertos.org/Documentation/02-Kernel/03-Supported-devices/04-Demos/ARM-Cortex/RTOS-Cortex-M3-M4)

CAN / inverter:

- [PM100 Datasheet](https://www.cascadiamotion.com/images/catalog/DataSheets/PM100.pdf)
- [Cascadia Motion CAN Protocol](https://app.box.com/s/vf9259qlaadhzxqiqrt5cco8xpsn84hk/file/27334613044)

# Related Projects

- [VCU GUI](https://github.com/sufst/vcu-gui)
- [VCU PCB](https://github.com/sufst/pcb)
- [RTCAN](https://github.com/sufst/rtcan)
- [CAN Bus Definitions](https://github.com/sufst/can-defs)
