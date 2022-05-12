###############################################################################
#               :
#   File        :   Makefile
#               :
#   Author(s)   :   Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
#               :
#   Description :   VCU makefile
#               :
###############################################################################

######################################
# configuration
######################################

TARGET = VCU
BUILD_DIR = build
DEBUG = 1

# defines
DEBUG_C_DEFS = \
-DDEBUG \
-DTX_INCLUDE_USER_DEFINE_FILE \
-DUSE_HAL_DRIVER \
-DSTM32H723xx \

RELEASE_C_DEFS = \
-DCOMPETITION_MODE=1 \
-DTX_INCLUDE_USER_DEFINE_FILE \
-DUSE_HAL_DRIVER \
-DSTM32H723xx \

DEBUG_ASM_DEFS = \

RELEASE_ASM_DEFS = \

# flags
ALWAYS_FLAGS = \
-Wall \
-Werror=implicit-function-declaration \
-fdata-sections \
-ffunction-sections \

DEBUG_C_FLAGS =  $(ALWAYS_FLAGS) -std=c11 -g -gdwarf-2
RELEASE_C_FLAGS = $(ALWAYS_FLAGS) -std=c11
DEBUG_ASM_FLAGS =  $(ALWAYS_FLAGS)
RELEASE_ASM_FLAGS = $(ALWAYS_FLAGS)

# optimisation
DEBUG_OPT = -Og 
RELEASE_OPT = -O3

# target MCU
CPU = -mcpu=cortex-m7
FPU = -mfpu=fpv5-d16
FLOAT_ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT_ABI)


#######################################
# toolchain
#######################################
PREFIX = arm-none-eabi-

CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S


#######################################
# linker
#######################################

LDSCRIPT = STM32H723ZGTx_FLASH.ld
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections


######################################
# sources
######################################

C_SOURCES =  \
SUFST/Src/ready_to_drive.c \
SUFST/Src/Test/throttle_testbench_data.c \
SUFST/Src/Test/testbench.c \
SUFST/Src/messaging_system.c \
SUFST/Src/rtc_time.c \
SUFST/Src/Threads/sensor_thread.c \
SUFST/Src/Threads/control_thread.c \
SUFST/Src/Threads/fault_state_thread.c \
SUFST/Src/Threads/can_thread.c \
SUFST/Src/CAN/pm100.c \
SUFST/Src/driver_profile_data.c \
SUFST/Src/config_rules.c \
SUFST/Src/CAN/can_types.c \
SUFST/Src/driver_profiles.c \
Core/Src/main.c \
Core/Src/gpio.c \
Core/Src/app_threadx.c \
Core/Src/adc.c \
Core/Src/fdcan.c \
Core/Src/rtc.c \
Core/Src/stm32h7xx_it.c \
Core/Src/stm32h7xx_hal_msp.c \
Core/Src/stm32h7xx_hal_timebase_tim.c \
AZURE_RTOS/App/app_azure_rtos.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fdcan.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c \
Core/Src/system_stm32h7xx.c \
Middlewares/ST/threadx/common/src/tx_initialize_high_level.c \
Middlewares/ST/threadx/common/src/tx_initialize_kernel_enter.c \
Middlewares/ST/threadx/common/src/tx_initialize_kernel_setup.c \
Middlewares/ST/threadx/common/src/tx_block_allocate.c \
Middlewares/ST/threadx/common/src/tx_block_pool_cleanup.c \
Middlewares/ST/threadx/common/src/tx_block_pool_create.c \
Middlewares/ST/threadx/common/src/tx_block_pool_delete.c \
Middlewares/ST/threadx/common/src/tx_block_pool_info_get.c \
Middlewares/ST/threadx/common/src/tx_block_pool_initialize.c \
Middlewares/ST/threadx/common/src/tx_block_pool_prioritize.c \
Middlewares/ST/threadx/common/src/tx_block_release.c \
Middlewares/ST/threadx/common/src/tx_byte_allocate.c \
Middlewares/ST/threadx/common/src/tx_byte_pool_cleanup.c \
Middlewares/ST/threadx/common/src/tx_byte_pool_create.c \
Middlewares/ST/threadx/common/src/tx_byte_pool_delete.c \
Middlewares/ST/threadx/common/src/tx_byte_pool_info_get.c \
Middlewares/ST/threadx/common/src/tx_byte_pool_initialize.c \
Middlewares/ST/threadx/common/src/tx_byte_pool_prioritize.c \
Middlewares/ST/threadx/common/src/tx_byte_pool_search.c \
Middlewares/ST/threadx/common/src/tx_byte_release.c \
Middlewares/ST/threadx/common/src/tx_event_flags_cleanup.c \
Middlewares/ST/threadx/common/src/tx_event_flags_create.c \
Middlewares/ST/threadx/common/src/tx_event_flags_delete.c \
Middlewares/ST/threadx/common/src/tx_event_flags_get.c \
Middlewares/ST/threadx/common/src/tx_event_flags_info_get.c \
Middlewares/ST/threadx/common/src/tx_event_flags_initialize.c \
Middlewares/ST/threadx/common/src/tx_event_flags_set.c \
Middlewares/ST/threadx/common/src/tx_event_flags_set_notify.c \
Middlewares/ST/threadx/common/src/tx_mutex_cleanup.c \
Middlewares/ST/threadx/common/src/tx_mutex_create.c \
Middlewares/ST/threadx/common/src/tx_mutex_delete.c \
Middlewares/ST/threadx/common/src/tx_mutex_get.c \
Middlewares/ST/threadx/common/src/tx_mutex_info_get.c \
Middlewares/ST/threadx/common/src/tx_mutex_initialize.c \
Middlewares/ST/threadx/common/src/tx_mutex_prioritize.c \
Middlewares/ST/threadx/common/src/tx_mutex_priority_change.c \
Middlewares/ST/threadx/common/src/tx_mutex_put.c \
Middlewares/ST/threadx/common/src/tx_queue_cleanup.c \
Middlewares/ST/threadx/common/src/tx_queue_create.c \
Middlewares/ST/threadx/common/src/tx_queue_delete.c \
Middlewares/ST/threadx/common/src/tx_queue_flush.c \
Middlewares/ST/threadx/common/src/tx_queue_front_send.c \
Middlewares/ST/threadx/common/src/tx_queue_info_get.c \
Middlewares/ST/threadx/common/src/tx_queue_initialize.c \
Middlewares/ST/threadx/common/src/tx_queue_prioritize.c \
Middlewares/ST/threadx/common/src/tx_queue_receive.c \
Middlewares/ST/threadx/common/src/tx_queue_send.c \
Middlewares/ST/threadx/common/src/tx_queue_send_notify.c \
Middlewares/ST/threadx/common/src/tx_semaphore_ceiling_put.c \
Middlewares/ST/threadx/common/src/tx_semaphore_cleanup.c \
Middlewares/ST/threadx/common/src/tx_semaphore_create.c \
Middlewares/ST/threadx/common/src/tx_semaphore_delete.c \
Middlewares/ST/threadx/common/src/tx_semaphore_get.c \
Middlewares/ST/threadx/common/src/tx_semaphore_info_get.c \
Middlewares/ST/threadx/common/src/tx_semaphore_initialize.c \
Middlewares/ST/threadx/common/src/tx_semaphore_prioritize.c \
Middlewares/ST/threadx/common/src/tx_semaphore_put.c \
Middlewares/ST/threadx/common/src/tx_semaphore_put_notify.c \
Middlewares/ST/threadx/common/src/tx_thread_create.c \
Middlewares/ST/threadx/common/src/tx_thread_delete.c \
Middlewares/ST/threadx/common/src/tx_thread_entry_exit_notify.c \
Middlewares/ST/threadx/common/src/tx_thread_identify.c \
Middlewares/ST/threadx/common/src/tx_thread_info_get.c \
Middlewares/ST/threadx/common/src/tx_thread_initialize.c \
Middlewares/ST/threadx/common/src/tx_thread_preemption_change.c \
Middlewares/ST/threadx/common/src/tx_thread_priority_change.c \
Middlewares/ST/threadx/common/src/tx_thread_relinquish.c \
Middlewares/ST/threadx/common/src/tx_thread_reset.c \
Middlewares/ST/threadx/common/src/tx_thread_resume.c \
Middlewares/ST/threadx/common/src/tx_thread_shell_entry.c \
Middlewares/ST/threadx/common/src/tx_thread_sleep.c \
Middlewares/ST/threadx/common/src/tx_thread_stack_analyze.c \
Middlewares/ST/threadx/common/src/tx_thread_stack_error_handler.c \
Middlewares/ST/threadx/common/src/tx_thread_stack_error_notify.c \
Middlewares/ST/threadx/common/src/tx_thread_suspend.c \
Middlewares/ST/threadx/common/src/tx_thread_system_preempt_check.c \
Middlewares/ST/threadx/common/src/tx_thread_system_resume.c \
Middlewares/ST/threadx/common/src/tx_thread_system_suspend.c \
Middlewares/ST/threadx/common/src/tx_thread_terminate.c \
Middlewares/ST/threadx/common/src/tx_thread_time_slice.c \
Middlewares/ST/threadx/common/src/tx_thread_time_slice_change.c \
Middlewares/ST/threadx/common/src/tx_thread_timeout.c \
Middlewares/ST/threadx/common/src/tx_thread_wait_abort.c \
Middlewares/ST/threadx/common/src/tx_time_get.c \
Middlewares/ST/threadx/common/src/tx_time_set.c \
Middlewares/ST/threadx/common/src/txe_block_allocate.c \
Middlewares/ST/threadx/common/src/txe_block_pool_create.c \
Middlewares/ST/threadx/common/src/txe_block_pool_delete.c \
Middlewares/ST/threadx/common/src/txe_block_pool_info_get.c \
Middlewares/ST/threadx/common/src/txe_block_pool_prioritize.c \
Middlewares/ST/threadx/common/src/txe_block_release.c \
Middlewares/ST/threadx/common/src/txe_byte_allocate.c \
Middlewares/ST/threadx/common/src/txe_byte_pool_create.c \
Middlewares/ST/threadx/common/src/txe_byte_pool_delete.c \
Middlewares/ST/threadx/common/src/txe_byte_pool_info_get.c \
Middlewares/ST/threadx/common/src/txe_byte_pool_prioritize.c \
Middlewares/ST/threadx/common/src/txe_byte_release.c \
Middlewares/ST/threadx/common/src/txe_event_flags_create.c \
Middlewares/ST/threadx/common/src/txe_event_flags_delete.c \
Middlewares/ST/threadx/common/src/txe_event_flags_get.c \
Middlewares/ST/threadx/common/src/txe_event_flags_info_get.c \
Middlewares/ST/threadx/common/src/txe_event_flags_set.c \
Middlewares/ST/threadx/common/src/txe_event_flags_set_notify.c \
Middlewares/ST/threadx/common/src/txe_mutex_create.c \
Middlewares/ST/threadx/common/src/txe_mutex_delete.c \
Middlewares/ST/threadx/common/src/txe_mutex_get.c \
Middlewares/ST/threadx/common/src/txe_mutex_info_get.c \
Middlewares/ST/threadx/common/src/txe_mutex_prioritize.c \
Middlewares/ST/threadx/common/src/txe_mutex_put.c \
Middlewares/ST/threadx/common/src/txe_queue_create.c \
Middlewares/ST/threadx/common/src/txe_queue_delete.c \
Middlewares/ST/threadx/common/src/txe_queue_flush.c \
Middlewares/ST/threadx/common/src/txe_queue_front_send.c \
Middlewares/ST/threadx/common/src/txe_queue_info_get.c \
Middlewares/ST/threadx/common/src/txe_queue_prioritize.c \
Middlewares/ST/threadx/common/src/txe_queue_receive.c \
Middlewares/ST/threadx/common/src/txe_queue_send.c \
Middlewares/ST/threadx/common/src/txe_queue_send_notify.c \
Middlewares/ST/threadx/common/src/txe_semaphore_ceiling_put.c \
Middlewares/ST/threadx/common/src/txe_semaphore_create.c \
Middlewares/ST/threadx/common/src/txe_semaphore_delete.c \
Middlewares/ST/threadx/common/src/txe_semaphore_get.c \
Middlewares/ST/threadx/common/src/txe_semaphore_info_get.c \
Middlewares/ST/threadx/common/src/txe_semaphore_prioritize.c \
Middlewares/ST/threadx/common/src/txe_semaphore_put.c \
Middlewares/ST/threadx/common/src/txe_semaphore_put_notify.c \
Middlewares/ST/threadx/common/src/txe_thread_create.c \
Middlewares/ST/threadx/common/src/txe_thread_delete.c \
Middlewares/ST/threadx/common/src/txe_thread_entry_exit_notify.c \
Middlewares/ST/threadx/common/src/txe_thread_info_get.c \
Middlewares/ST/threadx/common/src/txe_thread_preemption_change.c \
Middlewares/ST/threadx/common/src/txe_thread_priority_change.c \
Middlewares/ST/threadx/common/src/txe_thread_relinquish.c \
Middlewares/ST/threadx/common/src/txe_thread_reset.c \
Middlewares/ST/threadx/common/src/txe_thread_resume.c \
Middlewares/ST/threadx/common/src/txe_thread_suspend.c \
Middlewares/ST/threadx/common/src/txe_thread_terminate.c \
Middlewares/ST/threadx/common/src/txe_thread_time_slice_change.c \
Middlewares/ST/threadx/common/src/txe_thread_wait_abort.c \
Middlewares/ST/threadx/common/src/tx_timer_activate.c \
Middlewares/ST/threadx/common/src/tx_timer_change.c \
Middlewares/ST/threadx/common/src/tx_timer_create.c \
Middlewares/ST/threadx/common/src/tx_timer_deactivate.c \
Middlewares/ST/threadx/common/src/tx_timer_delete.c \
Middlewares/ST/threadx/common/src/tx_timer_expiration_process.c \
Middlewares/ST/threadx/common/src/tx_timer_info_get.c \
Middlewares/ST/threadx/common/src/tx_timer_initialize.c \
Middlewares/ST/threadx/common/src/tx_timer_system_activate.c \
Middlewares/ST/threadx/common/src/tx_timer_system_deactivate.c \
Middlewares/ST/threadx/common/src/tx_timer_thread_entry.c \
Middlewares/ST/threadx/common/src/txe_timer_activate.c \
Middlewares/ST/threadx/common/src/txe_timer_change.c \
Middlewares/ST/threadx/common/src/txe_timer_create.c \
Middlewares/ST/threadx/common/src/txe_timer_deactivate.c \
Middlewares/ST/threadx/common/src/txe_timer_delete.c \
Middlewares/ST/threadx/common/src/txe_timer_info_get.c  

ASM_SOURCES =  \
startup_stm32h723xx.s \
Core/Src/tx_initialize_low_level.s \
Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_restore.s \
Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_save.s \
Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_control.s \
Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.s \
Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_stack_build.s \
Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_system_return.s \
Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_timer_interrupt.s


#######################################
# includes
#######################################

ASM_INCLUDES = 

C_INCLUDES =  \
-ISUFST/Inc \
-ISUFST/Inc/CAN \
-ISUFST/Inc/Test \
-ISUFST/Inc/Threads \
-ICore/Inc \
-IAZURE_RTOS/App \
-IDrivers/STM32H7xx_HAL_Driver/Inc \
-IDrivers/STM32H7xx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32H7xx/Include \
-IDrivers/CMSIS/Include \
-IMiddlewares/ST/threadx/common/inc/ \
-IMiddlewares/ST/threadx/ports/cortex_m7/gnu/inc/ \
 

#######################################
# defines
#######################################

ifeq ($(DEBUG), 1)
	C_DEFS = $(DEBUG_C_DEFS)
	ASM_DEFS = $(DEBUG_ASM_DEFS)
else 
	C_DEFS = $(RELEASE_C_DEFS)
	ASM_DEFS = $(RELEASE_ASM_DEFS)
endif

#######################################
# flags
#######################################

ifeq ($(DEBUG), 1)
	OPT=$(DEBUG_OPT)
else
	OPT=$(RELEASE_OPT)
endif

ASFLAGS = $(MCU) $(ASM_DEFS) $(ASM_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections


#######################################
# objects
#######################################

# generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# list of ASM objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))


#######################################
# targets
#######################################

# default build all
all: 
	@make $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin \

# pre build
.PHONY: prebuild
prebuild:
	tput setaf 5; tput bold; echo "Compiling..."; tput sgr0

# C
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) prebuild
	echo "$<"
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

# ASM
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR) prebuild
	echo "$<"
	$(AS) -c $(CFLAGS) $< -o $@

# .elf 
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	tput setaf 5; tput bold; echo; echo "Linking..."; tput sgr0
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@
	tput setaf 2;  echo "$@"; echo; tput sgr0

# .hex
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR) prebuild
	$(HEX) $< $@

# .bin
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR) prebuild
	tput setaf 5; tput bold; echo "Creating binary..."; tput sgr0
	$(BIN) $< $@	
	tput setaf 2;  echo "$@"; echo; tput sgr0
	
# create build directory
$(BUILD_DIR):
	mkdir $@	

# clean
clean:
	tput setaf 5; tput bold; echo "Cleaning build directory..."
	-rm -fR $(BUILD_DIR)
	tput sgr0; tput setaf 2; echo "Done"; tput sgr0

# flash
flash: $(BUILD_DIR)/$(TARGET).bin
	tput setaf 5; tput bold; echo "Flashing..."; tput sgr0
	st-flash write $< 0x08000000
	echo

# check toolchain
toolchain:
	cd Scripts; ./toolchain.sh check \
	|| (echo; ./toolchain.sh install)
  

#######################################
# dependencies
#######################################

-include $(wildcard $(BUILD_DIR)/*.d)
