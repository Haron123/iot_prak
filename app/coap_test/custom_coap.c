#include "custom_coap.h"

static void saul_to_string(int id, char buf[64])
{
    saul_reg_t *saul = saul_reg_find_nth(id);
    const char* name = saul->name;
    snprintf(buf, 64, "%d: %s\n", id, name);
}

ssize_t _saul_list(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch(method_flag) 
    {
        case COAP_GET: ;
            /* Init coap response */
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

            /* Get the list of all Devices*/
            int id = 0;
            int offset = 0;
            saul_reg_t *cur_reg = saul_reg_find_nth(id);

            char temp_buf[64];
            while(cur_reg)
            {
                saul_to_string(id, temp_buf);
                cur_reg = saul_reg_find_nth(++id);

                /* Add List of devices to coap response */
                resp_len += fmt_str((char*)pdu->payload + offset, temp_buf);
                offset += strlen(temp_buf);
            }

            return resp_len;
    }

    return 0;
}

static void write_payload_to_saul(coap_pkt_t* pdu)
{
    int16_t id = atoi(strtok((char*)pdu->payload, ","));
    int16_t val1 = atoi(strtok(NULL, ","));
    int16_t val2 = atoi(strtok(NULL, ","));
    int16_t val3 = atoi(strtok(NULL, ","));
    phydat_t data = {{val1, val2, val3}, 0, 0};
    
    printf("id: %d\n", id);
    printf("val1: %d\n", val1);
    printf("val2: %d\n", val2);
    printf("val3: %d\n", val3);

    saul_reg_write(saul_reg_find_nth(id), &data);
}

ssize_t _saul_action(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch(method_flag) 
    {
        case COAP_GET: ;
            /* Get the device ID that was asked for*/
            int id = 0;
            if (1) 
            {
                char payload[] = {0, 0};
                memcpy(payload, (char*)pdu->payload, sizeof(payload));
                id = atoi(payload);
            }
            printf("Someone asked to read ID: %d\n", id);

            /* Get Information about device ID as JSON */
            char json_buf[64];
            read_saul_as_json(id, json_buf);

            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

            resp_len += fmt_str((char*)pdu->payload, json_buf);
            return resp_len;

        case COAP_PUT:
            write_payload_to_saul(pdu);
            return gcoap_response(NULL, NULL, 0, COAP_CODE_VALID);
    }

    return 0;
}

//static sock_udp_ep_t _proxy_remote;
//static char _proxy_uri[CONFIG_URI_MAX];

/* Retain request URI to re-request if response includes block. User must not
 * start a new request (with a new path) until any blockwise transfer
 * completes or times out. */
static char _last_req_uri[CONFIG_URI_MAX];

/* Last remote endpoint where an Observe request has been sent to */
//static sock_udp_ep_t obs_remote;

/* the token used for observing a remote resource */
//static uint8_t obs_req_token[GCOAP_TOKENLEN_MAX];

/* actual length of above token */
//static size_t obs_req_tkl = 0;

static void _my_resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    (void)remote;
    (void)memo;
    (void)pdu;
}

static int _uristr2remote(const char *uri, sock_udp_ep_t *remote, const char **path,
                          char *buf, size_t buf_len)
{
    if (strlen(uri) >= buf_len) {
        DEBUG_PUTS("URI too long");
        return 1;
    }
    uri_parser_result_t urip;
    if (uri_parser_process(&urip, uri, strlen(uri))) {
        DEBUG("'%s' is not a valid URI\n", uri);
        return 1;
    }
    memcpy(buf, urip.host, urip.host_len);
    buf[urip.host_len] = '\0';
    if (urip.port_str_len) {
        strcat(buf, ":");
        strncat(buf, urip.port_str, urip.port_str_len);
        buf[urip.host_len + 1 + urip.port_str_len] = '\0';
    }
    if (sock_udp_name2ep(remote, buf) != 0) {
        DEBUG("Could not resolve address '%s'\n", buf);
        return -1;
    }
    if (remote->port == 0) {
        remote->port = !strncmp("coaps", urip.scheme, 5) ? CONFIG_GCOAPS_PORT : CONFIG_GCOAP_PORT;
    }
    if (path) {
        *path = urip.path;
    }
    strcpy(buf, uri);
    return 0;
}

static gcoap_socket_type_t _get_tl(const char *uri)
{
    if (!strncmp(uri, "coaps", 5)) {
        return GCOAP_SOCKET_TYPE_DTLS;
    }
    else if (!strncmp(uri, "coap", 4)) {
        return GCOAP_SOCKET_TYPE_UDP;
    }
    return GCOAP_SOCKET_TYPE_UNDEF;
}

void coap_send_string(char *uri, char *payload, int method)
{
    int code_pos = method;

    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    ssize_t len;
    sock_udp_ep_t remote;

    //const char *uri = "coap://[::1]/saul/action";
    const char *path = "";

    if(_uristr2remote(uri, &remote, &path, _last_req_uri, sizeof(_last_req_uri)) != 0)
    {
        printf("str2remote error\n");
    }

    if(gcoap_req_init(&pdu, buf, CONFIG_GCOAP_PDU_BUF_SIZE, code_pos, NULL) < 0)
    {
        printf("init error\n");
    }

    coap_opt_add_uri_path(&pdu, path);
    coap_hdr_set_type(pdu.hdr, COAP_TYPE_NON);

    len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
    printf("len: %d\n", len);

    gcoap_socket_type_t tl = _get_tl(_last_req_uri);

    int payload_len = strlen(payload);
    memcpy(pdu.payload, payload, payload_len);
    len += payload_len;

    printf("%s\n", pdu.payload);
    
    len = gcoap_req_send(buf, len, &remote, NULL, _my_resp_handler, NULL, tl);

    printf("len: %d\n", len);
}