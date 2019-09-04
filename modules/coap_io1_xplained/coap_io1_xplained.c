#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "xtimer.h"
#include "periph/i2c.h"

#include "net/gcoap.h"

#include "coap_utils.h"
#include "coap_io1_xplained.h"

#ifdef MODULE_TFT_DISPLAY
#include "tft_display.h"
#endif

#define ENABLE_DEBUG (0)
#include "debug.h"

#define I2C_INTERFACE              I2C_DEV(0)    /* I2C interface number */
#define SENSOR_ADDR                (0x48 | 0x07) /* I2C temperature address on sensor */

static uint8_t response[64] = { 0 };

ssize_t io1_xplained_temperature_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;
    memset(response, 0, sizeof(response));
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    int16_t temperature;
    read_io1_xplained_temperature(&temperature);
    size_t p = 0;
    p += sprintf((char*)&response[p], "%i°C", temperature);
    response[p] = '\0';
    memcpy(pdu->payload, response, p);

    return gcoap_finish(pdu, p, COAP_FORMAT_TEXT);
}

void read_io1_xplained_temperature(int16_t *temperature)
{
    char buffer[2] = { 0 };
    /* read temperature register on I2C bus */
    if (i2c_read_bytes(I2C_INTERFACE, SENSOR_ADDR, buffer, 2, 0) < 0) {
        printf("Error: cannot read at address %i on I2C interface %i\n",
               SENSOR_ADDR, I2C_INTERFACE);
        return;
    }
    
    uint16_t data = (buffer[0] << 8) | buffer[1];
    int8_t sign = 1;
    /* Check if negative and clear sign bit. */
    if (data & (1 << 15)) {
        sign *= -1;
        data &= ~(1 << 15);
    }
    /* Convert to temperature */
    data = (data >> 5);
    *temperature = data * sign * 0.125;

    return;
}

void io1_xplained_handler(void *args)
{
    (void) args;

    int16_t temperature;
    read_io1_xplained_temperature(&temperature);
    ssize_t p1 = 0;
    ssize_t p2 = 0;
    p1 = sprintf((char*)&response[p1], "temperature:");
    p2 = sprintf((char*)&response[p1], "%i°C",
                    temperature);
    response[p1 + p2] = '\0';
#ifdef MODULE_TFT_DISPLAY
    display_send_buf(TFT_DISPLAY_TEMP, (uint8_t*) response + p1, p2);
#endif
    send_coap_post((uint8_t*)"/server", response);
}
