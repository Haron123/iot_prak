#ifndef CUSTOM_COAP_C_
#define CUSTOM_COAP_C_

#include "boardcontrol.h"
#include "net/gcoap.h"
#include "net/utils.h"
#include "fmt.h"
#include "gcoap_example.h"
#include <string.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "net/gcoap.h"
#include "net/sock/udp.h"
#include "net/sock/util.h"
#include "od.h"
#include "shell.h"
#include "uri_parser.h"

#include "gcoap_example.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#if IS_USED(MODULE_GCOAP_DTLS)
#include "net/dsm.h"
#endif

#ifndef CONFIG_URI_MAX
#define CONFIG_URI_MAX      128
#endif

ssize_t _saul_list(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
ssize_t _saul_action(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);

void set_led(void);

#endif // CUSTOM_COAP_C_