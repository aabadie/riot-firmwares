/*
 * Copyright (C) 2019 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "shell.h"

#include "net/nanocoap.h"
#include "net/gcoap.h"

/* RIOT firmware libraries */
#include "coap_common.h"
#include "coap_position.h"
#include "coap_ccs811.h"
#include "coap_bmx280.h"
#include "coap_sm_pwm_01c.h"
#include "schedreg.h"

#ifdef MODULE_COAP_SUIT
#include "suit/coap.h"
#include "riotboot/slot.h"
#include "coap_suit.h"
#endif

#ifdef MODULE_TFT_DISPLAY
#include "tft_display.h"
#endif

#define BMX280_SEND_INTERVAL        (6*US_PER_SEC)
#define CCS811_SEND_INTERVAL        (6*US_PER_SEC)
#define SM_PWM_01C_SEND_INTERVAL    (18*US_PER_SEC)
#define BEACON_SEND_INTERVAL        (30*US_PER_SEC)

static const shell_command_t shell_commands[] = {
    { NULL, NULL, NULL }
};

#define MAIN_QUEUE_SIZE       (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* import "ifconfig" shell command, used for printing addresses */
extern int _gnrc_netif_config(int argc, char **argv);

/* CoAP resources (alphabetical order) */
static const coap_resource_t _resources[] = {
    { "/board", COAP_GET, board_handler, NULL },
    { "/eco2", COAP_GET, ccs811_eco2_handler, NULL },
#ifdef MODULE_BME280
    { "/humidity", COAP_GET, bmx280_humidity_handler, NULL },
#endif
    { "/mcu", COAP_GET, mcu_handler, NULL },
    { "/name", COAP_GET, name_handler, NULL },
    { "/os", COAP_GET, os_handler, NULL },
    { "/position", COAP_GET, position_handler, NULL },
    { "/pressure", COAP_GET, bmx280_pressure_handler, NULL },
#ifdef MODULE_COAP_SUIT
    /* this line adds the whole "/suit"-subtree */
    SUIT_COAP_SUBTREE,
    { "/suit_state", COAP_GET, suit_state_handler, NULL },
#endif
    { "/temperature", COAP_GET, bmx280_temperature_handler, NULL },
    { "/tlp", COAP_GET, sm_pwm_01c_tlp_handler, NULL },
    { "/tsp", COAP_GET, sm_pwm_01c_tsp_handler, NULL },
    { "/tvoc", COAP_GET, ccs811_tvoc_handler, NULL },
#ifdef MODULE_COAP_SUIT
    { "/vendor", COAP_GET, vendor_handler, NULL },
    { "/version", COAP_GET, version_handler, NULL },
#endif
};

static gcoap_listener_t _listener = {
    (coap_resource_t *)&_resources[0],
    sizeof(_resources) / sizeof(_resources[0]),
    NULL,
    NULL
};

int main(void)
{
    puts("RIOT Air Monitor application");

    /* gnrc which needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("Waiting for address autoconfiguration...");
    xtimer_sleep(3);

    /* print network addresses */
    puts("Configured network interfaces:");
    _gnrc_netif_config(0, NULL);

    /* start coap server loop */
    gcoap_register_listener(&_listener);

#ifdef MODULE_COAP_SUIT
    printf("running from slot %u\n", riotboot_slot_current());
    riotboot_slot_print_hdr(riotboot_slot_current());
    /* start suit coap updater thread */
    suit_coap_run();
    init_suit_coap_msg_handler();
#endif

#ifdef MODULE_TFT_DISPLAY
    ucg_t ucg;
    /* start tft displays*/
    init_st7735_printer(&ucg);
#endif

    /* init schedreg thread */
    kernel_pid_t sched_pid = init_schedreg_thread();

    /* start beacon and register */
    init_beacon_sender();
    xtimer_t beacon_xtimer;
    msg_t beacon_msg;
    schedreg_t beacon_reg = SCHEDREG_INIT(beacon_handler, NULL, &beacon_msg,
                                          &beacon_xtimer, BEACON_SEND_INTERVAL);
    schedreg_register(&beacon_reg, sched_pid);

    /* start ccs811 and register */
    init_ccs811_sender(true, true);
    xtimer_t ccs811_xtimer;
    msg_t ccs811_msg;
    schedreg_t ccs811_reg = SCHEDREG_INIT(ccs811_handler, NULL, &ccs811_msg,
                                          &ccs811_xtimer, CCS811_SEND_INTERVAL);
    schedreg_register(&ccs811_reg, sched_pid);


    /* delay start of bmx280 so display shows at different time the sensors data*/
    xtimer_sleep(3);
    /* start bmx280 and register */
    init_bmx280_sender(true, true, true);
    xtimer_t bmx280_xtimer;
    msg_t bmx280_msg;
    schedreg_t bmx280_reg = SCHEDREG_INIT(bmx280_handler, NULL, &bmx280_msg,
                                          &bmx280_xtimer, BMX280_SEND_INTERVAL);
    schedreg_register(&bmx280_reg, sched_pid);

    /* delay start of sm_pwm_01c so display shows at different time the sensors data*/
    xtimer_sleep(3);
    /* start sm_pwm_01c and register */
    init_sm_pwm_01c_sender();
    xtimer_t sm_pwm_01c_xtimer;
    msg_t sm_pwm_01c_msg;
    schedreg_t sm_pwm_01c_reg = SCHEDREG_INIT(sm_pwm_01c_handler, NULL,
                                              &sm_pwm_01c_msg, &sm_pwm_01c_xtimer,
                                              SM_PWM_01C_SEND_INTERVAL);
    schedreg_register(&sm_pwm_01c_reg, sched_pid);

    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
