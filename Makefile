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
C_STANDARD = c11

# defines
ALWAYS_C_DEFS = \
-DTX_INCLUDE_USER_DEFINE_FILE \
-DUSE_HAL_DRIVER \
-DSTM32H723xx

DEBUG_C_DEFS = \
-DDEBUG \

RELEASE_C_DEFS = \
-DCOMPETITION_MODE=1 \

DEBUG_ASM_DEFS = \

RELEASE_ASM_DEFS = \

# flags
ALWAYS_FLAGS = \
-Wall \
-Wextra \
-pedantic \
-Wno-unused-parameter \
-Werror=implicit-function-declaration \
-fdata-sections \
-ffunction-sections \

DEBUG_C_FLAGS =  $(ALWAYS_FLAGS) -std=$(C_STANDARD) -g -gdwarf-2
RELEASE_C_FLAGS = $(ALWAYS_FLAGS) -std=$(C_STANDARD) -s
DEBUG_ASM_FLAGS =  $(ALWAYS_FLAGS)
RELEASE_ASM_FLAGS = $(ALWAYS_FLAGS)

# optimisation
DEBUG_OPT = -O0 
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

LDSCRIPT = src/STM32H723ZGTx_FLASH.ld
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

######################################
# sources
######################################

C_SOURCES =  \
src/SUFST/Src/init.c \
src/SUFST/Src/fault.c \
src/SUFST/Src/config_rules.c \
src/SUFST/Src/ready_to_drive.c \
src/SUFST/Src/apps.c \
src/SUFST/Src/scs.c \
src/SUFST/Src/bps.c \
src/SUFST/Src/shutdown.c \
src/SUFST/Src/CAN/can_device_state.c \
src/SUFST/Src/CAN/pm100.c \
src/SUFST/Src/CAN/drs.c \
src/SUFST/Src/Profiles/driver_profiles.c \
src/SUFST/Src/Profiles/driver_profile_data.c \
src/SUFST/Src/Test/testbench.c \
src/SUFST/Src/Test/apps_testbench_data.c \
src/SUFST/Src/Test/trace.c \
src/SUFST/Src/Threads/sensor_thread.c \
src/SUFST/Src/Threads/control_thread.c \
src/SUFST/Src/Threads/watchdog_thread.c \
src/SUFST/Src/Threads/can_tx_thread.c \
src/SUFST/Src/Threads/can_rx_thread.c \
src/SUFST/Src/Threads/init_thread.c \
src/Core/Src/main.c \
src/Core/Src/gpio.c \
src/Core/Src/app_threadx.c \
src/Core/Src/adc.c \
src/Core/Src/fdcan.c \
src/Core/Src/rtc.c \
src/Core/Src/stm32h7xx_it.c \
src/Core/Src/stm32h7xx_hal_msp.c \
src/Core/Src/stm32h7xx_hal_timebase_tim.c \
src/AZURE_RTOS/App/app_azure_rtos.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fdcan.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c \
src/Core/Src/system_stm32h7xx.c \
src/Middlewares/ST/threadx/common/src/tx_initialize_high_level.c \
src/Middlewares/ST/threadx/common/src/tx_initialize_kernel_enter.c \
src/Middlewares/ST/threadx/common/src/tx_initialize_kernel_setup.c \
src/Middlewares/ST/threadx/common/src/tx_block_allocate.c \
src/Middlewares/ST/threadx/common/src/tx_block_pool_cleanup.c \
src/Middlewares/ST/threadx/common/src/tx_block_pool_create.c \
src/Middlewares/ST/threadx/common/src/tx_block_pool_delete.c \
src/Middlewares/ST/threadx/common/src/tx_block_pool_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_block_pool_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_block_pool_prioritize.c \
src/Middlewares/ST/threadx/common/src/tx_block_release.c \
src/Middlewares/ST/threadx/common/src/tx_byte_allocate.c \
src/Middlewares/ST/threadx/common/src/tx_byte_pool_cleanup.c \
src/Middlewares/ST/threadx/common/src/tx_byte_pool_create.c \
src/Middlewares/ST/threadx/common/src/tx_byte_pool_delete.c \
src/Middlewares/ST/threadx/common/src/tx_byte_pool_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_byte_pool_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_byte_pool_prioritize.c \
src/Middlewares/ST/threadx/common/src/tx_byte_pool_search.c \
src/Middlewares/ST/threadx/common/src/tx_byte_release.c \
src/Middlewares/ST/threadx/common/src/tx_event_flags_cleanup.c \
src/Middlewares/ST/threadx/common/src/tx_event_flags_create.c \
src/Middlewares/ST/threadx/common/src/tx_event_flags_delete.c \
src/Middlewares/ST/threadx/common/src/tx_event_flags_get.c \
src/Middlewares/ST/threadx/common/src/tx_event_flags_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_event_flags_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_event_flags_set.c \
src/Middlewares/ST/threadx/common/src/tx_event_flags_set_notify.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_cleanup.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_create.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_delete.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_get.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_prioritize.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_priority_change.c \
src/Middlewares/ST/threadx/common/src/tx_mutex_put.c \
src/Middlewares/ST/threadx/common/src/tx_queue_cleanup.c \
src/Middlewares/ST/threadx/common/src/tx_queue_create.c \
src/Middlewares/ST/threadx/common/src/tx_queue_delete.c \
src/Middlewares/ST/threadx/common/src/tx_queue_flush.c \
src/Middlewares/ST/threadx/common/src/tx_queue_front_send.c \
src/Middlewares/ST/threadx/common/src/tx_queue_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_queue_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_queue_prioritize.c \
src/Middlewares/ST/threadx/common/src/tx_queue_receive.c \
src/Middlewares/ST/threadx/common/src/tx_queue_send.c \
src/Middlewares/ST/threadx/common/src/tx_queue_send_notify.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_ceiling_put.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_cleanup.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_create.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_delete.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_get.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_prioritize.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_put.c \
src/Middlewares/ST/threadx/common/src/tx_semaphore_put_notify.c \
src/Middlewares/ST/threadx/common/src/tx_thread_create.c \
src/Middlewares/ST/threadx/common/src/tx_thread_delete.c \
src/Middlewares/ST/threadx/common/src/tx_thread_entry_exit_notify.c \
src/Middlewares/ST/threadx/common/src/tx_thread_identify.c \
src/Middlewares/ST/threadx/common/src/tx_thread_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_thread_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_thread_preemption_change.c \
src/Middlewares/ST/threadx/common/src/tx_thread_priority_change.c \
src/Middlewares/ST/threadx/common/src/tx_thread_relinquish.c \
src/Middlewares/ST/threadx/common/src/tx_thread_reset.c \
src/Middlewares/ST/threadx/common/src/tx_thread_resume.c \
src/Middlewares/ST/threadx/common/src/tx_thread_shell_entry.c \
src/Middlewares/ST/threadx/common/src/tx_thread_sleep.c \
src/Middlewares/ST/threadx/common/src/tx_thread_stack_analyze.c \
src/Middlewares/ST/threadx/common/src/tx_thread_stack_error_handler.c \
src/Middlewares/ST/threadx/common/src/tx_thread_stack_error_notify.c \
src/Middlewares/ST/threadx/common/src/tx_thread_suspend.c \
src/Middlewares/ST/threadx/common/src/tx_thread_system_preempt_check.c \
src/Middlewares/ST/threadx/common/src/tx_thread_system_resume.c \
src/Middlewares/ST/threadx/common/src/tx_thread_system_suspend.c \
src/Middlewares/ST/threadx/common/src/tx_thread_terminate.c \
src/Middlewares/ST/threadx/common/src/tx_thread_time_slice.c \
src/Middlewares/ST/threadx/common/src/tx_thread_time_slice_change.c \
src/Middlewares/ST/threadx/common/src/tx_thread_timeout.c \
src/Middlewares/ST/threadx/common/src/tx_thread_wait_abort.c \
src/Middlewares/ST/threadx/common/src/tx_time_get.c \
src/Middlewares/ST/threadx/common/src/tx_time_set.c \
src/Middlewares/ST/threadx/common/src/txe_block_allocate.c \
src/Middlewares/ST/threadx/common/src/txe_block_pool_create.c \
src/Middlewares/ST/threadx/common/src/txe_block_pool_delete.c \
src/Middlewares/ST/threadx/common/src/txe_block_pool_info_get.c \
src/Middlewares/ST/threadx/common/src/txe_block_pool_prioritize.c \
src/Middlewares/ST/threadx/common/src/txe_block_release.c \
src/Middlewares/ST/threadx/common/src/txe_byte_allocate.c \
src/Middlewares/ST/threadx/common/src/txe_byte_pool_create.c \
src/Middlewares/ST/threadx/common/src/txe_byte_pool_delete.c \
src/Middlewares/ST/threadx/common/src/txe_byte_pool_info_get.c \
src/Middlewares/ST/threadx/common/src/txe_byte_pool_prioritize.c \
src/Middlewares/ST/threadx/common/src/txe_byte_release.c \
src/Middlewares/ST/threadx/common/src/txe_event_flags_create.c \
src/Middlewares/ST/threadx/common/src/txe_event_flags_delete.c \
src/Middlewares/ST/threadx/common/src/txe_event_flags_get.c \
src/Middlewares/ST/threadx/common/src/txe_event_flags_info_get.c \
src/Middlewares/ST/threadx/common/src/txe_event_flags_set.c \
src/Middlewares/ST/threadx/common/src/txe_event_flags_set_notify.c \
src/Middlewares/ST/threadx/common/src/txe_mutex_create.c \
src/Middlewares/ST/threadx/common/src/txe_mutex_delete.c \
src/Middlewares/ST/threadx/common/src/txe_mutex_get.c \
src/Middlewares/ST/threadx/common/src/txe_mutex_info_get.c \
src/Middlewares/ST/threadx/common/src/txe_mutex_prioritize.c \
src/Middlewares/ST/threadx/common/src/txe_mutex_put.c \
src/Middlewares/ST/threadx/common/src/txe_queue_create.c \
src/Middlewares/ST/threadx/common/src/txe_queue_delete.c \
src/Middlewares/ST/threadx/common/src/txe_queue_flush.c \
src/Middlewares/ST/threadx/common/src/txe_queue_front_send.c \
src/Middlewares/ST/threadx/common/src/txe_queue_info_get.c \
src/Middlewares/ST/threadx/common/src/txe_queue_prioritize.c \
src/Middlewares/ST/threadx/common/src/txe_queue_receive.c \
src/Middlewares/ST/threadx/common/src/txe_queue_send.c \
src/Middlewares/ST/threadx/common/src/txe_queue_send_notify.c \
src/Middlewares/ST/threadx/common/src/txe_semaphore_ceiling_put.c \
src/Middlewares/ST/threadx/common/src/txe_semaphore_create.c \
src/Middlewares/ST/threadx/common/src/txe_semaphore_delete.c \
src/Middlewares/ST/threadx/common/src/txe_semaphore_get.c \
src/Middlewares/ST/threadx/common/src/txe_semaphore_info_get.c \
src/Middlewares/ST/threadx/common/src/txe_semaphore_prioritize.c \
src/Middlewares/ST/threadx/common/src/txe_semaphore_put.c \
src/Middlewares/ST/threadx/common/src/txe_semaphore_put_notify.c \
src/Middlewares/ST/threadx/common/src/txe_thread_create.c \
src/Middlewares/ST/threadx/common/src/txe_thread_delete.c \
src/Middlewares/ST/threadx/common/src/txe_thread_entry_exit_notify.c \
src/Middlewares/ST/threadx/common/src/txe_thread_info_get.c \
src/Middlewares/ST/threadx/common/src/txe_thread_preemption_change.c \
src/Middlewares/ST/threadx/common/src/txe_thread_priority_change.c \
src/Middlewares/ST/threadx/common/src/txe_thread_relinquish.c \
src/Middlewares/ST/threadx/common/src/txe_thread_reset.c \
src/Middlewares/ST/threadx/common/src/txe_thread_resume.c \
src/Middlewares/ST/threadx/common/src/txe_thread_suspend.c \
src/Middlewares/ST/threadx/common/src/txe_thread_terminate.c \
src/Middlewares/ST/threadx/common/src/txe_thread_time_slice_change.c \
src/Middlewares/ST/threadx/common/src/txe_thread_wait_abort.c \
src/Middlewares/ST/threadx/common/src/tx_timer_activate.c \
src/Middlewares/ST/threadx/common/src/tx_timer_change.c \
src/Middlewares/ST/threadx/common/src/tx_timer_create.c \
src/Middlewares/ST/threadx/common/src/tx_timer_deactivate.c \
src/Middlewares/ST/threadx/common/src/tx_timer_delete.c \
src/Middlewares/ST/threadx/common/src/tx_timer_expiration_process.c \
src/Middlewares/ST/threadx/common/src/tx_timer_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_timer_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_timer_system_activate.c \
src/Middlewares/ST/threadx/common/src/tx_timer_system_deactivate.c \
src/Middlewares/ST/threadx/common/src/tx_timer_thread_entry.c \
src/Middlewares/ST/threadx/common/src/txe_timer_activate.c \
src/Middlewares/ST/threadx/common/src/txe_timer_change.c \
src/Middlewares/ST/threadx/common/src/txe_timer_create.c \
src/Middlewares/ST/threadx/common/src/txe_timer_deactivate.c \
src/Middlewares/ST/threadx/common/src/txe_timer_delete.c \
src/Middlewares/ST/threadx/common/src/txe_timer_info_get.c \
src/Middlewares/ST/threadx/common/src/tx_trace_buffer_full_notify.c \
src/Middlewares/ST/threadx/common/src/tx_trace_disable.c \
src/Middlewares/ST/threadx/common/src/tx_trace_enable.c \
src/Middlewares/ST/threadx/common/src/tx_trace_event_filter.c \
src/Middlewares/ST/threadx/common/src/tx_trace_event_unfilter.c \
src/Middlewares/ST/threadx/common/src/tx_trace_initialize.c \
src/Middlewares/ST/threadx/common/src/tx_trace_interrupt_control.c \
src/Middlewares/ST/threadx/common/src/tx_trace_isr_enter_insert.c \
src/Middlewares/ST/threadx/common/src/tx_trace_isr_exit_insert.c \
src/Middlewares/ST/threadx/common/src/tx_trace_object_register.c \
src/Middlewares/ST/threadx/common/src/tx_trace_object_unregister.c \
src/Middlewares/ST/threadx/common/src/tx_trace_user_event_insert.c

ASM_SOURCES =  \
src/startup_stm32h723xx.s \
src/Core/Src/tx_initialize_low_level.s \
src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_restore.s \
src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_save.s \
src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_control.s \
src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.s \
src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_stack_build.s \
src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_system_return.s \
src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_timer_interrupt.s

#######################################
# includes
#######################################

ASM_INCLUDES = 

C_INCLUDES =  \
-Isrc/SUFST/Inc \
-Isrc/SUFST/Inc/CAN \
-Isrc/SUFST/Inc/Profiles \
-Isrc/SUFST/Inc/Test \
-Isrc/SUFST/Inc/Threads \
-Isrc/SUFST/Inc/Utility \
-Isrc/Core/Inc \
-Isrc/AZURE_RTOS/App \
-Isrc/Drivers/STM32H7xx_HAL_Driver/Inc \
-Isrc/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy \
-Isrc/Drivers/CMSIS/Device/ST/STM32H7xx/Include \
-Isrc/Drivers/CMSIS/Include \
-Isrc/Middlewares/ST/threadx/ports/cortex_m7/gnu/inc/ \
-Isrc/Middlewares/ST/threadx/common/inc/ \

#######################################
# defines
#######################################

ifeq ($(DEBUG), 1)
	C_DEFS = $(ALWAYS_C_DEFS) $(DEBUG_C_DEFS)
	ASM_DEFS = $(DEBUG_ASM_DEFS)
else
	C_DEFS = $(ALWAYS_C_DEFS) $(RELEASE_C_DEFS)
	ASM_DEFS = $(RELEASE_ASM_DEFS)
endif

#######################################
# flags
#######################################

ifeq ($(DEBUG), 1)
	OPT=$(DEBUG_OPT)
	CFLAGS=$(DEBUG_C_FLAGS)
else
	OPT=$(RELEASE_OPT)
	CFLAGS=$(RELEASE_C_FLAGS)
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
	make $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin \

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
