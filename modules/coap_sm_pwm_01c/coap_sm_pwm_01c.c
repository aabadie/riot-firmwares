#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "xtimer.h"

#include "net/gcoap.h"

#include "coap_utils.h"
#include "coap_sm_pwm_01c.h"

#include "sm_pwm_01c.h"
#include "sm_pwm_01c_params.h"

#ifdef MODULE_TFT_DISPLAY
#include "tft_display.h"
#endif

#define ENABLE_DEBUG (0)
#include "debug.h"

static sm_pwm_01c_t sm_pwm_01c_dev;
static uint8_t response[64] = { 0 };

ssize_t sm_pwm_01c_tlp_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;
    memset(response, 0, sizeof(response));
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    int16_t tsp = sm_pwm_01c_read_tsp(&sm_pwm_01c_dev);
    size_t p = 0;
    p += sprintf((char*)&response[p], "%i ug/m3", tsp);
    response[p] = '\0';
    memcpy(pdu->payload, response, p);

    return gcoap_finish(pdu, p, COAP_FORMAT_TEXT);
}

ssize_t sm_pwm_01c_tsp_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;
    memset(response, 0, sizeof(response));
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    int16_t tlp = sm_pwm_01c_read_tlp(&sm_pwm_01c_dev);
    size_t p = 0;
    p += sprintf((char*)&response[p], "%i ug/m3", tlp);
    response[p] = '\0';
    memcpy(pdu->payload, response, p);

    return gcoap_finish(pdu, p, COAP_FORMAT_TEXT);
}

void sm_pwm_01c_handler(void *args)
{
    (void) args;

    ssize_t p1 = 0;
    ssize_t p2 = 0;
    int16_t tsp = sm_pwm_01c_read_tsp(&sm_pwm_01c_dev);

    p1 = sprintf((char*)&response[p1], "tsp:");
    p2 = sprintf((char*)&response[p1], "%.4ug/m3", tsp);
    response[p1 + p2] = '\0';
#ifdef MODULE_TFT_DISPLAY
    display_send_buf(TFT_DISPLAY_TSP, (uint8_t*) response + p1, p2);
#endif
    send_coap_post((uint8_t*)"/server", response);

    int16_t tlp = sm_pwm_01c_read_tlp(&sm_pwm_01c_dev);

    p1 = sprintf((char*)&response[p1], "tlp:");
    p2 = sprintf((char*)&response[p1], "%.4ug/m3", tlp);
    response[p1 + p2] = '\0';
#ifdef MODULE_TFT_DISPLAY
    display_send_buf(TFT_DISPLAY_TLP, (uint8_t*) response + p1, p2);
#endif
    send_coap_post((uint8_t*)"/server", response);

}

int init_sm_pwm_01c_sender(void)
{
    /* Initialize the SM_PWM_01C sensor */
    printf("+----------Initializing SM_PWM_01C sensor ------------+\n");
    if (sm_pwm_01c_init(&sm_pwm_01c_dev,  &sm_pwm_01c_params[0]) != 0) {
        puts("init device [ERROR]");
        return -1;
    }
    else {
        printf("Initialization successful\n\n");
    }
    sm_pwm_01c_start(&sm_pwm_01c_dev);

    return 0;
}