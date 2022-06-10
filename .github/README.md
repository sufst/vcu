<h1 align="center"> VCU - Vehicle Control Unit </h2>

# Table of Contents
- [About](#about)  
- [Contributing](#contributing)   
- [Useful Resources](#useful-resources)   
- [Related Projects](#related-projects)   

# About

The VCU is responsible for taking driver throttle inputs and communicating with the inverter.


# Contributing

## Conventional Commits
Commits (and issue/pull request titles) should follow the [conventional commits specification](https://www.conventionalcommits.org/en/v1.0.0/) with the format:
```
<type>(<scope>): <subject>
```

`<type>` __must__ be one of:
| Type       | Description                                                         |
|------------|---------------------------------------------------------------------|
| `feat`     | New features                                                        |
| `fix`      | Bug fixes                                                           |
| `refactor` | Refactors (rewriting/restructuring code without changing behaviour) |
| `perf`     | Refactors specifically to improve performance                       |
| `style`    | Code formatting, whitespace etc                                     |
| `test`     | New tests or changes to existing tests                              |
| `docs`     | Changes to documentation                                            |
| `build`    | Changes to build system, compilation, toolchain                     |      
| `chore`    | Chores (internal/organisational changes, updating .gitignore, etc)  |
| `misc`     | Anything which absolutely does not match any of the other types     |

`<scope>` should be a single word describing a part of the system such as (but not limited to):
| Scope      | Description                                                    |
|------------|----------------------------------------------------------------|
| `can`      | Communication with devices on the CAN bus                      |
| `control`  | Control algorithms (throttle mapping, traction control...)     |
| `drs`      | Drag reduction system                                          |
| `io`       | Reading/writing/configuring general I/O                        |
| `rtos`     | Real-time operating system (configuration, thread creation...) |
| `safety`   | Safety checks, fault handling, ready-to-drive logic            |
| `sensors`  | Sensor inputs                                                  |

`<subject>` should written as "lowercase, imperative, present tense" (e.g. _'create control thread'_ or _'add init.c to Makefile'_).

## Code Formatting
Code should be formatted before committing using [`trunk`](https://docs.trunk.io) with the command:
```
trunk fmt
```
Install `trunk` for Linux or macOS with:
```
curl https://get.trunk.io -fsSL | bash
```
Alternatively, install the [VS Code extension](https://marketplace.visualstudio.com/items?itemName=Trunk.io).

# Useful Resources

Documentation and rules:
- [VCU Wiki](https://github.com/sufst/vcu/wiki)
- [SUFST Docs Site](https://docs.sufst.co.uk/en/latest/)
- [FSUK 2022 rules](https://www.imeche.org/docs/default-source/1-oscar/formula-student/2022/rules/fsuk-2022-rules---final.pdf?sfvrsn=2)

Microcontroller:
- [STM32H723ZG MCU Datasheet](https://www.st.com/resource/en/datasheet/stm32h723vg.pdf)
- [STM32H7 Nucleo 144 Dev Board User Manual](https://www.st.com/resource/en/user_manual/dm00499160-stm32h7-nucleo144-boards-mb1364-stmicroelectronics.pdf)
- [STM32H7xx HAL Manual](https://www.st.com/resource/en/user_manual/um2217-description-of-stm32h7-hal-and-lowlayer-drivers-stmicroelectronics.pdf)

ThreadX RTOS:
- [ThreadX Overview](https://docs.microsoft.com/en-us/azure/rtos/threadx/overview-threadx)
- [ThreadX API](https://docs.microsoft.com/en-us/azure/rtos/threadx/chapter4)

CAN / inverter:
- [PM100 Datasheet](https://www.cascadiamotion.com/images/catalog/DataSheets/PM100.pdf)
- [Cascadia Motion CAN Protocol](https://app.box.com/s/vf9259qlaadhzxqiqrt5cco8xpsn84hk/file/27334613044)


# Related Projects

- [VCU Driver Profiles App](https://github.com/sufst/vcu-driver-profile)
- [VCU Breakout Board](https://github.com/sufst/vcu-breakout)
