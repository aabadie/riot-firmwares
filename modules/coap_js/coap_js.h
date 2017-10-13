#ifndef COAP_JS_H
#define COAP_JS_H

#include <inttypes.h>

#include "net/gcoap.h"

#ifdef __cplusplus
extern "C" {
#endif

ssize_t js_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* COAP_JS_H */
