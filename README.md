<h1 align="center"> VCU - Vehicle Control Unit </h2>

# Table of Contents

- [About](#about)
- [Development Environment](#development-environment)
- [Contributing](#contributing)
- [Useful Resources](#useful-resources)
- [Related Projects](#related-projects)

# About

The VCU is responsible for controlling the tractive system based on driver
input. The microcontroller used in this project is the STM32 F746ZG.

More detailed information about this project is available to members on the
SUFST Docs Site.

# Setup / Development Environment

## Submodules

This project depends on middlewares in the `src/Middlewares/` folder, some of
which are Git submodules. When first cloning this repo, run the following
commands:

```sh
git submodule init
git submodule update
```

For more information on submodules, see the [Git submodules documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules).

## Building and Flashing

To build this project and flash code to the microcontroller, you will need the
following on your path:

- [Make](https://www.gnu.org/software/make/)
- [ARM GNU Embedded Toolchain](https://developer.arm.com/downloads/-/gnu-rm)
- [STLink Open Source Toolset](https://github.com/stlink-org/stlink)
- `mkdir`, `rm`, `tput` and `echo`

Build with:

```sh
make -j -s
```

Flash with:

```sh
make flash
```

For detailed toolchain setup instructions, see the SUFST Docs Site.

> Note: Windows users should run these commands from Git Bash.

## VS Code

This project is set up to be edited and debugged in [VS Code](https://code.visualstudio.com).
The `.vscode`folder includes tasks and launch configurations. To improve
Intellisense, it is recommended to use [`ccdgen`](https://github.com/t-bre/ccdgen)
to generate a `compile_commands.json` database with the following command:

```sh
python -m ccdgen --extension .c .s --compiler arm-none-eabi-gcc -- make
```

Since the toolchain is set up to be fully command line based, it is also
possible to use other code editors.

> Note: Windows users should set Git Bash as the shell in VS Code.

## STM32CubeMX

[STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) is used
to generate boilerplate initialisation code for the microcontroller. These
configurations are stored in `src/VCU.ioc` which should not be edited manually.
To minimise the chance of merge conflicts, changes to the `.ioc` should be made
as infrequently as possible as the `.ioc` format is not well suited to version
control.

> Note: CubeMX generates a Makefile in the `src` folder. This should _not_ be
> used, there is a custom Makefile in the project root. If CubeMX adds something
> to the Makefile it generates which is not in the custom Makefile, it should be
> copied over. This should only happen infrequently when adding a new peripheral
> or CubeMX managed middlewares.

# Contributing

Before contributing to this project, make sure to familiarise yourself with the
[project-specific contributing guidelines](.github/CONTRIBUTING_EXTRA.md).

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
