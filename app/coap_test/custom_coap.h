#ifndef CUSTOM_COAP_C_
#define CUSTOM_COAP_C_

#include "boardcontrol.h"
#include "net/gcoap.h"
#include "net/utils.h"
#include "fmt.h"
#include <string.h>

ssize_t _saul_list(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
ssize_t _saul_action(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);

#endif // CUSTOM_COAP_C_