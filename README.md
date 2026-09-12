<h1 align="center"> VCU - Vehicle Control Unit </h2>

# About

The VCU is responsible for controlling the tractive system based on driver
input. The microcontroller used in this project is the STM32 F746ZG.

More detailed information about this project is available to members on the
SUFST Docs Site.

# Navigation

- [Testing with Vector CANoe](./docs/canoe-testing.md)
- [Code Architecture Overview](./docs/code-overview.md)
- [Development Environment Setup](./docs/dev-env-setup.md)
- [GitHub Project-Specific Contributing Guidelines](./.github/CONTRIBUTING_EXTRA.md)

# Useful Resources

Microcontroller:

- [STM32F746ZG MCU Datasheet](https://www.st.com/resource/en/datasheet/stm32f746zg.pdf)
- [STM32F7xx HAL Manual](https://www.st.com/resource/en/user_manual/dm00189702-description-of-stm32f7-hal-and-lowlayer-drivers-stmicroelectronics.pdf)

ThreadX RTOS:

- [ThreadX Overview](https://docs.microsoft.com/en-us/azure/rtos/threadx/overview-threadx)
- [ThreadX API](https://docs.microsoft.com/en-us/azure/rtos/threadx/chapter4)

CAN / inverter:

- [PM100 Datasheet](https://www.cascadiamotion.com/images/catalog/DataSheets/PM100.pdf)
- [Cascadia Motion CAN Protocol](https://app.box.com/s/vf9259qlaadhzxqiqrt5cco8xpsn84hk/file/27334613044)

# Related Projects

- [VCU GUI](https://github.com/sufst/vcu-gui)
- [VCU PCB](https://github.com/sufst/pcb)
- [RTCAN](https://github.com/sufst/rtcan)
- [CAN Bus Definitions](https://github.com/sufst/can-defs)
