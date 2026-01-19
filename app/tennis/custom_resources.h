#ifndef CUSTOM_RESOURCES_H_
#define CUSTOM_RESOURCES_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "event/periodic_callback.h"
#include "event/thread.h"
#include "fmt.h"
#include "net/gcoap.h"
#include "net/utils.h"
#include "od.h"
#include "periph/rtc.h"
#include "shell.h"
#include "time_units.h"

#include "gcoap_example.h"
#include "stdatomic.h"
#include "gamestate.h"
#include "coap_client.h"
#include "logging.h"

#ifdef __cplusplus
}
#endif

#endif // CUSTOM_RESOURCES_H_

ssize_t _ready_to_duel(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
ssize_t _start_duel(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
ssize_t _end_duel(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
ssize_t _lobby_id(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
ssize_t _hit_received(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
ssize_t _heartbeat(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
