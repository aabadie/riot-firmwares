#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net/gcoap.h"

#include "coap_js.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

extern char script[];
extern void js_restart(void);

ssize_t _blockwise_script_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len)
{
    DEBUG("_blockwise_script_handler()\n");

    uint32_t result = COAP_CODE_204;
    uint32_t blknum;
    unsigned szx;
    int res = coap_get_blockopt(pkt, COAP_OPT_BLOCK1, &blknum, &szx);
    if (res >= 0) {
        DEBUG("blknum=%u blksize=%u more=%u\n", (unsigned)blknum, coap_szx2size(szx), res);
        size_t offset = blknum << (szx + 4);
        DEBUG("received bytes %u-%u\n", (unsigned)offset, (unsigned)offset+pkt->payload_len);

        /* overwrite the current script with the new received script  */
        memcpy(script + offset, (char *)pkt->payload, pkt->payload_len);
        if (res) {
            result = COAP_CODE_231;
        }
        else {
            script[offset + pkt->payload_len] = '\0';
            DEBUG("script received (blockwise):\n");
            DEBUG("-----\n");
            DEBUG(script);
            DEBUG("\n-----\n");
            DEBUG("restarting js.\n");
            js_restart();
        }
    }
    else {
        memcpy(script, (char *)pkt->payload, pkt->payload_len);
        script[pkt->payload_len] = '\0';
        DEBUG("script received:\n");
        DEBUG("-----\n");
        DEBUG(script);
        DEBUG("\n-----\n");
        DEBUG("restarting js.\n");
        js_restart();
    }

    ssize_t reply_len = coap_build_reply(pkt, result, buf, len, 0);
    uint8_t *pkt_pos = (uint8_t*)pkt->hdr + reply_len;
    if (res >= 0) {
        pkt_pos += coap_put_option_block1(pkt_pos, 0, blknum, szx, res);
    }
    return pkt_pos - (uint8_t*)pkt->hdr;
}

ssize_t js_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len)
{
    ssize_t rsp_len = 0;
    unsigned code = COAP_CODE_EMPTY;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pkt));

    switch (method_flag) {
        case COAP_GET:
            code = COAP_CODE_205;
            rsp_len = strlen((char *)script);
            break;
        case COAP_POST:
        case COAP_PUT:
        {
            return _blockwise_script_handler(pkt, buf, len);
        }
    }

    return coap_reply_simple(pkt, code, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)script, rsp_len);
}
