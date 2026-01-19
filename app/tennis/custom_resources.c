#include "custom_resources.h"

ssize_t _ready_to_duel(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
	(void)ctx;
    printf("Asked if im ready\n");
    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));
    switch(method_flag) 
    {
        case COAP_GET: ;
            /* Init coap response */
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

			if(gs.ready_to_duel)
			{
				resp_len += fmt_str((char*)pdu->payload, "true");
			}
			else
			{
				resp_len += fmt_str((char*)pdu->payload, "false");
			}

            return resp_len;
    }

    return 0;
}

// Expects URI to be the payload
static char partner_uri[IPV6_ADDR_MAX_STR_LEN*2];
static sock_udp_ep_t partner_udp;
ssize_t _start_duel(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch(method_flag) 
    {
        case COAP_POST:
            LOGINFO("URI %s wants to start Duel", (char*)pdu->payload);

            /* Init coap response */
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

			if(gs.ready_to_duel)
			{
				resp_len += fmt_str((char*)pdu->payload, "true");
                strcpy(partner_uri, (char*)pdu->payload);
                memcpy(&partner_udp, ctx->remote, sizeof(sock_udp_ep_t));
                build_partner_uris(partner_uri);
                LOGINFO("Accepted duel");
                gs.ready_to_duel = false;
                gs.in_game = true;

                if(gs.is_lobby)
                {
                    unregister_lobby();
                }
			}
			else
			{
                LOGINFO("Declined duel");
				resp_len += fmt_str((char*)pdu->payload, "false");
			}

            return resp_len;
    }

    return 0;
}

static bool sock_udp_t_same(sock_udp_ep_t* first, sock_udp_ep_t* second)
{
    if(first->port != second->port ||
    first->addr.ipv6 != second->addr.ipv6 ||
    first->family != second->family ||
    first->netif != second->netif)
    {
        return false;
    }

    return true;
}

ssize_t _end_duel(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));
    
    switch(method_flag) 
    {
        case COAP_POST:
            const char* result;
            if(sock_udp_t_same(ctx->remote, &partner_udp))
            {
                result = "false";
            }
            else
            {
                result = "true";
                gs.ready_to_duel = true;
            }

            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

			resp_len += fmt_str((char*)pdu->payload, result);

            return resp_len;
    }

    return 0;
}

char hwaddr_str[64];
uint8_t hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
ssize_t _lobby_id(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
	unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));
    (void)ctx;

    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    int res = netif_get_opt(&netif->netif, NETOPT_ADDRESS, 0, hwaddr, sizeof(hwaddr));
    l2util_addr_to_str(hwaddr, res, hwaddr_str);

    switch(method_flag) 
    {
        case COAP_GET:
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

            LOGINFO("hwaddr_str: %s", hwaddr_str);
            fmt_str(gs.lobby_id, hwaddr_str);
			resp_len += fmt_str((char*)pdu->payload, hwaddr_str);

            return resp_len;
    }

    return 0;
}

ssize_t _hit_received(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));
    (void)ctx;

    switch(method_flag) 
    {
        case COAP_POST:
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);
			resp_len += fmt_str((char*)pdu->payload, "Ack");
            LOGINFO("Other player did a hit!");

            return resp_len;
    }

    return 0;
}

ssize_t _heartbeat(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));
    (void)ctx;

    switch(method_flag) 
    {
        case COAP_POST:
            gs.heartbeat_timer = 0;
            LOGINFO("Got heartbeat");
            ssize_t resp_len = gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);

            return resp_len;
    }

    return 0;
}