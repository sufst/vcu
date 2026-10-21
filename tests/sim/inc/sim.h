#ifndef SIM_H
#define SIM_H
#include <stdint.h>
#include "main.h"
void sim_set_input(GPIO_TypeDef *port, uint16_t pin, int level);
int sim_get_output(GPIO_TypeDef *port, uint16_t pin);
void sim_push_rx(int bus, uint32_t id, const uint8_t *data, uint8_t len);
void sim_on_tx(int bus, uint32_t id, const uint8_t *data, uint8_t len);
extern volatile unsigned sim_tx_count[3];
extern volatile unsigned sim_log_lines;
extern volatile unsigned sim_log_lines_no_ram;
extern int sim_quiet_log;
extern volatile uint16_t sim_adc1[5];
extern volatile uint16_t sim_adc2[1];

/* SD card / USB stubs (stubs.c) */
extern volatile unsigned sim_sd_writes;
extern volatile unsigned sim_usb_init_calls;

/* watchdog model (stubs.c) */
extern volatile int sim_wdg_started;
extern volatile unsigned sim_wdg_kicks;
extern volatile uint32_t sim_wdg_max_gap_ms;  // longest time between two kicks
extern volatile uint32_t sim_wdg_expired_at;  // tick it would have reset the VCU (0 = never)
void sim_wdg_reset_gap(void);

/* name of a task whose creation should fail (to test init failure) */
extern const char *volatile sim_fail_task_create;

#include "vcu.h"
void sim_freertos_init_failure_path(vcu_context_t *vcu_ptr);
#endif
