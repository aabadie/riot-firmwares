/*
 * Copyright (C) 2017 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdio.h>
#include <string.h>

#include "xtimer.h"

#include "net/loramac.h"
#include "rn2xx3_params.h"
#include "rn2xx3.h"

#include "bmx280_params.h"
#include "bmx280.h"

#define WAIT_INTERVAL (120U)

#ifndef APPLICATION_NAME
#define APPLICATION_NAME "TTN LoRa node"
#endif

static rn2xx3_t rn2xx3_dev;
static bmx280_t bmx280_dev;
static char msg[64];

void _send_msg(const char *resource, const char *value)
{
    ssize_t p = 0;
    memset(msg, 0, strlen(msg));
    p += sprintf(msg, "{\"r\":\"%s\",\"v\":\"%s\"}", resource, value);
    msg[p] = '\0';

    rn2xx3_mac_tx(&rn2xx3_dev, (uint8_t*)msg, strlen(msg));
    xtimer_sleep(WAIT_INTERVAL);
}

int main(void)
{
    puts("RIOT TTN Node application");

    /* Initialize the BME280 sensor */
    printf("+------------Initializing BMX280 sensor ------------+\n");
    int result = bmx280_init(&bmx280_dev, &bmx280_params[0]);
    if (result == -1) {
        puts("[Error] BMX280: The given i2c is not enabled");
        return -1;
    }
    else if (result == -2) {
        puts("[Error] BMX280: The sensor did not answer correctly on the given address");
        return -1;
    }
    else {
        puts("BMX280: Initialization succeeded\n");
    }

    rn2xx3_setup(&rn2xx3_dev, &rn2xx3_params[0]);
    if (rn2xx3_init(&rn2xx3_dev) < 0) {
        puts("[Error] RN2XX3: Initialization failed");
        return -1;
    }

    if (rn2xx3_mac_join_network(&rn2xx3_dev, LORAMAC_DEFAULT_JOIN_PROCEDURE) != RN2XX3_REPLY_JOIN_ACCEPTED) {
        puts("Error] RN2XX3: Join procedure failed.");
        return -1;
    }

    puts("RN2XX3: Initialization succeeded.");

    _send_msg("name", APPLICATION_NAME);
    _send_msg("os", "riot");

    while (1) {
        char value_str[16];
        ssize_t p = 0;
        int16_t temperature = bmx280_read_temperature(&bmx280_dev);
        p += sprintf(value_str, "%d.%d°C",
                     temperature / 100, (temperature % 100) /10);
        value_str[p] = '\0';
        _send_msg("temperature", value_str);

        memset(value_str, 0, strlen(value_str));
        p = 0;
        uint32_t pressure = bmx280_read_pressure(&bmx280_dev);
        p += sprintf(value_str, "%lu.%d",
                     (unsigned long)pressure / 100, (int)pressure % 100);
        value_str[p] = '\0';
        _send_msg("pressure", value_str);
    }

    return 0;
}
