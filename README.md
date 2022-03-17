# VCU - Vehicle Control Unit
The VCU is responsible for taking driver throttle inputs and communicating with the inverter.


## System Outline
```mermaid
graph TB

    classDef QueueClass fill:#cc99ff,stroke-width:0px;
    
    subgraph Initialisation
        IH[Hardware Initialisation]
        IT[Thread Creation]
        IR[Ready-to-Drive Procedure]
        IK[RTOS Kernel Entry]
        IH --> IT --> IR --> IK
    end
    
    IK --> SE & CE & BE

    subgraph "Sensor Thread&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
        SE[Thread Entry]--> SR
        SR[Read Throttle Input]
        SV[Validate Input]
        SS[Scale Input]
        SM[Send Message]
        SR --> SV --> SS --> SM --> SR
    end
  
    subgraph "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Control Thread"
        CE[Thread Entry]
        CW[Wait for Input]
        CD[Apply Driver Profile]
        CT[Create Torque Request]
        CM[Send Torque Request]
        CE --> CW --> CD --> CT --> CM --> CW
    end

    subgraph "CAN Thread&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
        BE[Thread Entry]
        BW[Wait for Tx Request]
        BG[Generate CAN Message]
        BT[Transmit CAN Message]
        BE --> BW --> BG --> BT --> BW
    end

    SM -- Throttle --> QT
    QT -- Throttle --> CW
    CM -- Torque Request --> QC
    QC -- CAN Tx Request --> BW

    QC[CAN Priority Queue]:::QueueClass
    QT[Throttle FIFO Queue]:::QueueClass
    
```


## Useful Resources

Documentation:
- [VCU Wiki](https://github.com/sufst/vcu/wiki)
- [SUFST Docs Site](https://docs.sufst.co.uk/en/latest/)

Microcontroller:
- [STM32H723ZG MCU Datasheet](https://www.st.com/resource/en/datasheet/stm32h723vg.pdf)
- [STM32H7 Nucleo 144 Dev Board User Manual](https://www.st.com/resource/en/user_manual/dm00499160-stm32h7-nucleo144-boards-mb1364-stmicroelectronics.pdf)
- [STM32H7xx HAL Manual](https://www.st.com/resource/en/user_manual/um2217-description-of-stm32h7-hal-and-lowlayer-drivers-stmicroelectronics.pdf)

ThreadX RTOS:
- [ThreadX Overview](https://docs.microsoft.com/en-us/azure/rtos/threadx/overview-threadx)
- [ThreadX API](https://docs.microsoft.com/en-us/azure/rtos/threadx/chapter4)


## Related Projects

- [VCU Driver Profiles App](https://github.com/sufst/vcu-driver-profile)
- [VCU Breakout Board](https://github.com/sufst/vcu-breakout)
