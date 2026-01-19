#ifndef COAP_CLIENT_H_
#define COAP_CLIENT_H_

#ifdef __cplusplus
extern "C"
{
#endif

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
#include "net/cord/common.h"
#include "net/cord/config.h"
#include "net/cord/ep.h"
#include "sema.h"
#include "gamestate.h"
#include "logging.h"

#ifdef MODULE_CORD_EP_STANDALONE
#include "net/cord/ep_standalone.h"
#endif

#include "gcoap_example.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#if IS_USED(MODULE_GCOAP_DTLS)
#include "net/dsm.h"
#endif

#ifndef CONFIG_URI_MAX
#define CONFIG_URI_MAX      128
#endif

	bool is_resource_from_me(char *uri);

	/**
	 * @brief Gets the IP from a COAP URI
	 * @note dst has to be atleast IPV6_ADDR_MAX_STR_LEN
	 */
	void get_ip_from_uri(char *uri, char *dst);

	char *get_my_global_ipv6_str(void);

	int find_players(void);

	bool start_duel(char *uri);

	void end_duel(char *uri);

	void heartbeat(char *uri);

	void register_new_resource(char *uri);

	char *get_lobby_id(void);

	int find_hit_uri(void);

	void post_hit(uint32_t strenth);

	bool is_player_ready(char *uri);

	void update_resources(void);

	int update_rd(void);
	sock_udp_ep_t *get_rd_remote(void);
	char *get_rd_path(void);
	void register_to_rd(void);
	void coap_send_buf_cb(char *uri, uint8_t *payload, int payload_len, int method, int type, gcoap_resp_handler_t resp_handler);
	void coap_send_buf(char *uri, uint8_t *payload, int payload_len, int method, int type);
	void coap_send_string(char *uri, char *payload, int method);
	void build_partner_uris(char *partner_uri);

	int register_lobby(void);

	int unregister_lobby(void);

#ifdef __cplusplus
}
#endif


#endif // COAP_CLIENT_H_