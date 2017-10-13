/*
 * Copyright (C) 2017 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"
#include "xtimer.h"
#include "js.h"

/* include headers generated from *.js */
#include "lib.js.h"
#include "local.js.h"

static event_queue_t event_queue;

char script[2048];

#include "nanocoap.h"
#include "net/gcoap.h"

#include "coap_common.h"
#include "coap_js.h"

static const shell_command_t shell_commands[] = {
    { NULL, NULL, NULL }
};

#define MAIN_QUEUE_SIZE       (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* import "ifconfig" shell command, used for printing addresses */
extern int _netif_config(int argc, char **argv);

/* CoAP resources (alphabetical order) */
static const coap_resource_t _resources[] = {
    { "/board", COAP_GET, board_handler },
    { "/js", COAP_GET | COAP_PUT | COAP_POST, js_handler },
    { "/mcu", COAP_GET, mcu_handler },
    { "/name", COAP_GET, name_handler },
    { "/os", COAP_GET, os_handler },
};

static gcoap_listener_t _listener = {
    (coap_resource_t *)&_resources[0],
    sizeof(_resources) / sizeof(_resources[0]),
    NULL
};

void js_start(event_t *unused)
{
    (void)unused;

    size_t script_len = strlen(script);
    if (script_len) {
        puts("Initializing jerryscript engine...");
        js_init();

        puts("Executing lib.js...");
        js_run(lib_js, lib_js_len);

        puts("Executing local.js...");
        js_run(local_js, local_js_len);

        puts("Executing script...");
        js_run((jerry_char_t*)script, script_len);
    }
    else {
        puts("Emtpy script, not exetcuting.");
    }
}

static event_t js_start_event = { .callback=js_start };

void js_restart(void)
{
    js_shutdown(&js_start_event);
}

int main(void)
{
    puts("RIOT Javacript OTA Node application");

    /* gnrc which needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("Waiting for address autoconfiguration...");
    xtimer_sleep(3);

    /* print network addresses */
    puts("Configured network interfaces:");
    _netif_config(0, NULL);

    /* start coap server loop */
    gcoap_register_listener(&_listener);
    init_beacon_sender();

    puts("setting up event queue");
    event_queue_init(&event_queue);
    js_event_queue = &event_queue;

    puts("Entering event loop...");
    event_loop(&event_queue);

    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
