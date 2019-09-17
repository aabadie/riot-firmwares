#ifndef COAP_SM_PWM_01C_H
#define COAP_SM_PWM_01C_H

#include <inttypes.h>

#include "net/gcoap.h"

#ifdef __cplusplus
extern "C" {
#endif

ssize_t sm_pwm_01c_tlp_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx);

ssize_t sm_pwm_01c_tsp_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx);

void sm_pwm_01c_handler(void *args);

int init_sm_pwm_01c_sender(void);

#ifdef __cplusplus
}
#endif

#endif /* COAP_SM_PWM_01C_H */
