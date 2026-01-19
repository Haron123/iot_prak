#include "coap_client.h"

/* Retain request URI to re-request if response includes block. User must not
 * start a new request (with a new path) until any blockwise transfer
 * completes or times out. */
static char _last_req_uri[CONFIG_URI_MAX];

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

static int check_resp_memo(const gcoap_request_memo_t *memo)
{
    if (memo->state == GCOAP_MEMO_TIMEOUT) 
    {
        LOGWARN("gcoap: timeout");
        return -1;
    }
    else if (memo->state == GCOAP_MEMO_RESP_TRUNC) 
    {
        /* The right thing to do here would be to look into whether at least
         * the options are complete, then to mentally trim the payload to the
         * next block boundary and pretend it was sent as a Block2 of that
         * size. */
        LOGWARN("gcoap: warning, incomplete response; continuing with the truncated payload");
        return -2;
    }
    else if (memo->state != GCOAP_MEMO_RESP) {
        LOGWARN("gcoap: error in response");
        return -1;
    }

    return 0;
}

static sema_t _update_resources_sema;
static void _save_rd_lookup(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    check_resp_memo(memo);
    (void)pdu;
    (void)remote;

    LOGINFO("Got: %s", pdu->payload);

    gs.num_lobbies = 0;
    char* left_arr_pos  = NULL;
    char* right_arr_pos = NULL;
    char* pointer_offset = (char*)pdu->payload;
    while ((left_arr_pos = strchr(pointer_offset, '<')) &&
       (right_arr_pos = strchr(pointer_offset, '>')))
    {
        int uri_len = right_arr_pos - left_arr_pos - 1;
        memcpy(gs.lobby_arr[gs.num_lobbies], left_arr_pos+1, uri_len);
        pointer_offset = left_arr_pos + uri_len+2;
        gs.num_lobbies++;
    }

    LOGINFO("Found %d resources:\n", gs.num_lobbies);
    for(int i = 0; i < gs.num_lobbies; i++)
    {
        LOGINFO("%s", gs.lobby_arr[i]);
    }
    sema_post(&_update_resources_sema);
}

void update_resources(void)
{
    sema_create(&_update_resources_sema, 0);
    // Replace with get_remote when it works
    char *uri = "coap://[ff05::fe]/resource-lookup/";

    coap_send_buf_cb(uri, NULL, 0, COAP_GET, COAP_TYPE_CON, _save_rd_lookup);
    sema_wait(&_update_resources_sema);
}

bool is_resource_from_me(char* uri)
{
    char uri_ip[IPV6_ADDR_MAX_STR_LEN];
    get_ip_from_uri(uri, uri_ip);

    int res = strcmp(uri_ip, get_my_global_ipv6_str());

    return res == 0;
}

void get_ip_from_uri(char* uri, char* dst)
{
    int left_arr_pos = strchr(uri, '[')  - uri;
    int right_arr_pos = strchr(uri, ']') - uri;
    int ip_len = right_arr_pos - left_arr_pos - 1;

    if(left_arr_pos < 0 || right_arr_pos < 0 || ip_len == 0)
    {
        LOGWARN("Could not parse ip from uri");
        return;
    }
    else if((unsigned int)ip_len > IPV6_ADDR_MAX_STR_LEN)
    {
        LOGWARN("Nothing writtent to dst, ip_len > max allowed len");
        return;
    }

    memcpy(dst, uri+left_arr_pos+1, ip_len);
    dst[ip_len] = '\0';
}

#ifndef NETIF_PRINT_IPV6_NUMOF
#define NETIF_PRINT_IPV6_NUMOF 4
#endif

char* get_my_global_ipv6_str(void)
{
    static char my_global_ipv6_str[IPV6_ADDR_MAX_STR_LEN];

    netif_t *netif = 0;
    while ((netif = netif_iter(netif)) != NULL) 
    {
        ipv6_addr_t addrs[NETIF_PRINT_IPV6_NUMOF];
        ssize_t num = netif_get_ipv6(netif, addrs, ARRAY_SIZE(addrs));
        if (num > 0) 
        {
            for(int i = 0; i < num; i++)
            {
                if(ipv6_addr_is_global(addrs + i))
                {
                    ipv6_addr_to_str(my_global_ipv6_str, addrs + i, IPV6_ADDR_MAX_STR_LEN);
                    return my_global_ipv6_str;
                }
            }
        }
    }

    return NULL;
}

// Returns number of resource from lobby_arr, where there is a player
int find_players(void)
{
    for(int i = 0; i < gs.num_lobbies; i++)
    {
        if(strstr(gs.lobby_arr[i], "lobby"))
        {
            if(!is_resource_from_me(gs.lobby_arr[i]))
            {
                if(is_player_ready(gs.lobby_arr[i]))
                {
                    return i;
                }
            }
        }
    }

    return -1;
}

int find_unused_lobby_id(void)
{
    return 0;
}

static sema_t start_duel_sem;
static bool start_duel_res;
static void _start_duel_cb(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    check_resp_memo(memo);
    (void)remote;

    if(pdu->payload[0] == 'f')
    {
        start_duel_res = false;
    }
    else if(pdu->payload[0] == 't')
    {
        start_duel_res = true;
    }
    else
    {
        LOGWARN("Info:");
        for(int i = 0; i < pdu->payload_len; i++)
        {
            LOGWARN("0x%X", pdu->payload[i]);
        }
        printf("\n");
        LOGWARN("Received wrong msg format %s", pdu->payload);
        start_duel_res = false;
    }

    sema_post(&start_duel_sem);
}

bool start_duel(char* uri)
{
    char myuri[IPV6_ADDR_MAX_STR_LEN*2];
    snprintf(myuri, sizeof(myuri), "coap://[%s]/hit", get_my_global_ipv6_str());

    sema_create(&start_duel_sem, 0);

    coap_send_buf_cb(uri, (uint8_t*)myuri, strlen(myuri), COAP_POST, COAP_TYPE_CON, _start_duel_cb);

    sema_wait(&start_duel_sem);

    gs.in_game = true;
    return start_duel_res;
}

void end_duel(char* uri)
{
    coap_send_buf_cb(uri, NULL, 0, COAP_POST, COAP_TYPE_NON, NULL);
}

void heartbeat(char* uri)
{
    coap_send_buf_cb(uri, NULL, 0, COAP_POST, COAP_TYPE_CON, NULL);
}

static sema_t get_lobby_id_sem;
static void _get_lobby_id_cb(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    check_resp_memo(memo);
    (void)remote;

    if(pdu->payload_len > sizeof(gs.lobby_id))
    {
        LOGWARN("Lobby id too big\n");
        sema_post(&get_lobby_id_sem);
        return;
    }

    memset(gs.lobby_id, '\0', sizeof(gs.lobby_id));
    memcpy(gs.lobby_id, (char*)pdu->payload, pdu->payload_len);
    LOGINFO("get_lobby_id: %s", pdu->payload);
    
    sema_post(&get_lobby_id_sem);
}

char* get_lobby_id(void)
{
    sema_create(&get_lobby_id_sem, 0);

    coap_send_buf_cb(gs.partner_lobby_id_uri, NULL, 0, COAP_GET, COAP_TYPE_CON, _get_lobby_id_cb);

    sema_wait(&get_lobby_id_sem);

    return gs.lobby_id;
}

int find_hit_uri(void)
{
    for(int i = 0; i < gs.num_lobbies; i++)
    {
        if(strstr(gs.lobby_arr[i], "gui_hit"))
        {
            return i;
        }
    }

    return -1;
}

void post_hit(uint32_t strength)
{
    char* uri = gs.lobby_arr[find_hit_uri()];
    printf("Sending Hit to %s\n and %s\n", uri, gs.partner_hit_happend_uri);

    uint8_t buf[1 + 4 + 6];
    buf[0] = gs.is_lobby; // 1
    memcpy(buf+1, &strength, sizeof(strength)); // 4
    memcpy(buf+5, gs.lobby_id, 6); // 6

    coap_send_buf(uri, buf, sizeof(buf), COAP_POST, COAP_TYPE_CON);
    coap_send_buf(gs.partner_hit_happend_uri, NULL, 0, COAP_POST, COAP_TYPE_CON);
}

static sema_t is_player_ready_sem;
static bool is_player_ready_res;
static void _is_player_ready_cb(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    check_resp_memo(memo);
    (void)remote;

    if(pdu->payload[0] == 'f')
    {
        is_player_ready_res = false;
    }
    else if(pdu->payload[0] == 't')
    {
        is_player_ready_res = true;
    }
    else
    {
        LOGWARN("Info:\n");
        for(int i = 0; i < pdu->payload_len; i++)
        {
            LOGWARN("0x%X\n", pdu->payload[i]);
        }
        printf("\n");
        LOGWARN("Received wrong msg format %s\n", pdu->payload);
        is_player_ready_res = false;
    }

    sema_post(&is_player_ready_sem);
}

bool is_player_ready(char* uri)
{
    sema_create(&is_player_ready_sem, 0);

    coap_send_buf_cb(uri, NULL, 0, COAP_GET, COAP_TYPE_CON, _is_player_ready_cb);

    sema_wait(&is_player_ready_sem);

    return is_player_ready_res;
}

static void print_ipv6(const uint8_t* ipv6) {
   printf("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                 (int)ipv6[0], (int)ipv6[1],
                 (int)ipv6[2], (int)ipv6[3],
                 (int)ipv6[4], (int)ipv6[5],
                 (int)ipv6[6], (int)ipv6[7],
                 (int)ipv6[8], (int)ipv6[9],
                 (int)ipv6[10], (int)ipv6[11],
                 (int)ipv6[12], (int)ipv6[13],
                 (int)ipv6[14], (int)ipv6[15]);
}

static sema_t update_rd_sem;
static int update_rd_res = -1;
static void _extract_rd_from_payload_handler(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    if(check_resp_memo(memo) < 0)
    {
        LOGERROR("Error");
        update_rd_res = -1;
        sema_post(&update_rd_sem);
        return;
    }

    LOGINFO("Got reply from RD");
    print_ipv6(remote->addr.ipv6);
    printf(":%X\n", remote->port);
    
    gs.rd_remote.family = remote->family;
    gs.rd_remote.netif  = remote->netif;
    gs.rd_remote.port   = remote->port;
    memcpy(gs.rd_remote.addr.ipv6, remote->addr.ipv6, sizeof(remote->addr.ipv6));

    int left_arr_pos = strchr((char*)pdu->payload, '<')  - (char*)pdu->payload;
    int right_arr_pos = strchr((char*)pdu->payload, '>') - (char*)pdu->payload;
    int uri_len = right_arr_pos - left_arr_pos - 1;

    if(left_arr_pos < 0 || right_arr_pos < 0)
    {
        LOGERROR("Error");
        update_rd_res = -1;
        sema_post(&update_rd_sem);
        return;
    }

    memcpy(gs.rd_path, pdu->payload+left_arr_pos+1, uri_len);

    gs.rd_valid = true;
    update_rd_res = 0;
    sema_post(&update_rd_sem);
}

int update_rd(void)
{
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    ssize_t len;
    sock_udp_ep_t remote;
    const char *path;
    
    sema_create(&update_rd_sem, 0);

    // Replace with multicast when it works
    char *uri = "coap://[ff05::fe]/.well-known/core";

    if(_uristr2remote(uri, &remote, &path, _last_req_uri, sizeof(_last_req_uri)) != 0){ printf("str2remote error\n"); }
    if(gcoap_req_init(&pdu, buf, sizeof(buf), COAP_METHOD_GET, NULL) < 0){ printf("init error\n"); }

    coap_opt_add_uri_path(&pdu, path);
    coap_opt_add_uri_query(&pdu, "rt", "core.rd");
    coap_hdr_set_type(pdu.hdr, COAP_TYPE_NON);
    
    len = coap_opt_finish(&pdu, COAP_OPT_FINISH_NONE);

    int res = gcoap_req_send(buf, len, &remote, NULL, _extract_rd_from_payload_handler, NULL, GCOAP_SOCKET_TYPE_UNDEF);
    gs.rd_remote = remote;
    if(res <= 0){ printf("Error getting RD URI\n"); }
    sema_wait(&update_rd_sem);

    return update_rd_res;
}

sock_udp_ep_t* get_rd_remote(void)
{
    if(gs.rd_valid)
    {
        return &gs.rd_remote;
    }
    else
    {
        return NULL;
    }
}

char* get_rd_path(void)
{
    if(gs.rd_valid)
    {
        return gs.rd_path;
    }
    else
    {
        return NULL;
    }
}

//static void add_resource_to_payload(uint8_t* payload, int payload_len)
//{
//    (void)payload;
//    (void)payload_len;
//}

void register_to_rd(void)
{
    sock_udp_ep_t remote;
    const char *path;

    /* Add the directory to the uri of the RD TODO: replace with get_rd_remote once it works */
    char uri_buf[64];
    char *uri = "coap://[ff05::fe]"; 
    sprintf(uri_buf, "%s%s", uri, get_rd_path());

    /* Convert the URI String to a remote struct and path string */
    if(_uristr2remote(uri_buf, &remote, &path, _last_req_uri, sizeof(_last_req_uri)) != 0){ printf("str2remote error\n"); }
   
    cord_ep_register(&remote, get_rd_path());
}

static void _my_resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    (void)remote;
    (void)memo;
    (void)pdu;

    check_resp_memo(memo);
    
    if(pdu->payload_len)
    {
        LOGINFO("Received %d Bytes reply:", pdu->payload_len);
        if(pdu->payload_len == strlen((char*)pdu->payload))
        {
            printf("%s\n", pdu->payload);
        }
        else
        {
            LOGINFO("Received %d Bytes reply:", pdu->payload_len);
            for(int i = 0; i < pdu->payload_len; i++)
            {  
                printf("0x%X, ", pdu->payload[i]);
                if(i % 5 == 0 && i != 0)
                {
                    printf("\n");
                }
            }
            printf("\n");
        }
    }
    else
    {
        printf("Received empty reply!\n");
    }
}

void coap_send_buf_cb(char *uri, uint8_t *payload, int payload_len, int method, int type, gcoap_resp_handler_t resp_handler)
{
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    ssize_t len;
    sock_udp_ep_t remote;

    const char *path = "";

    if(_uristr2remote(uri, &remote, &path, _last_req_uri, sizeof(_last_req_uri)) != 0)
    {
        LOGERROR("str2remote error");
    }

    if(gcoap_req_init(&pdu, buf, sizeof(buf), method, NULL) < 0)
    {
        LOGERROR("init error");
    }

    coap_opt_add_uri_path(&pdu, path);
    coap_hdr_set_type(pdu.hdr, type);
    
    if(payload_len > 0)
    {
        len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
        memcpy(pdu.payload, payload, payload_len);
        len += payload_len;
    }
    else
    {
        len = coap_opt_finish(&pdu, COAP_OPT_FINISH_NONE);
    }

    int res = gcoap_req_send(buf, len, &remote, NULL, resp_handler, NULL, GCOAP_SOCKET_TYPE_UNDEF);

    if(res <= 0)
    {
        LOGERROR("Error %d", res);
    }
}

void coap_send_buf(char *uri, uint8_t *payload, int payload_len, int method, int type)
{
    coap_send_buf_cb(uri, payload, payload_len, method, type, NULL);
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
        LOGERROR("str2remote error");
    }

    if(gcoap_req_init(&pdu, buf, CONFIG_GCOAP_PDU_BUF_SIZE, code_pos, NULL) < 0)
    {
        LOGERROR("init error");
    }

    coap_opt_add_uri_path(&pdu, path);
    coap_hdr_set_type(pdu.hdr, COAP_TYPE_NON);

    len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);

    gcoap_socket_type_t tl = _get_tl(_last_req_uri);

    int payload_len = strlen(payload);
    memcpy(pdu.payload, payload, payload_len);
    len += payload_len;

    printf("Sending COAP Packet with LEN: %d\n", len);
    printf("and Messeage: %s\n", payload);
    
    len = gcoap_req_send(buf, len, &remote, NULL, _my_resp_handler, NULL, tl);
}

static char uri_ip[IPV6_ADDR_MAX_STR_LEN];
void build_partner_uris(char *lobby_uri)
{
    get_ip_from_uri(lobby_uri, uri_ip);
    snprintf(gs.partner_start_duel_uri,  RESOURCE_LENGTH, "coap://[%s]/start_duel",  uri_ip);
    snprintf(gs.partner_end_duel_uri,    RESOURCE_LENGTH, "coap://[%s]/end_duel",    uri_ip);
    snprintf(gs.partner_hit_happend_uri, RESOURCE_LENGTH, "coap://[%s]/hit",         uri_ip);
    snprintf(gs.partner_lobby_id_uri,    RESOURCE_LENGTH, "coap://[%s]/lobby_id",    uri_ip);
    snprintf(gs.heartbeat_uri,           RESOURCE_LENGTH, "coap://[%s]/heartbeat",  uri_ip);
}

static uint8_t buf[512];
int register_lobby(void)
{
    if(!gs.rd_valid)
    {
        LOGWARN("register_lobby called with no valid rd");
        return -1;
    }

    int res;
    ssize_t pkt_len;
    coap_pkt_t pkt;

    /* build and send CoAP POST request to the RD's registration interface */
    res = gcoap_req_init(&pkt, buf, sizeof(buf), COAP_METHOD_POST, get_rd_path());
    if (res < 0) 
    {
        return CORD_EP_ERR;
    }
    /* set some packet options and write query string */
    coap_hdr_set_type(pkt.hdr, COAP_TYPE_CON);
    coap_opt_add_uint(&pkt, COAP_OPT_CONTENT_FORMAT, COAP_FORMAT_LINK);
    res = cord_common_add_qstring(&pkt);
    if (res < 0) 
    {
        return CORD_EP_ERR;
    }

    pkt_len = coap_opt_finish(&pkt, COAP_OPT_FINISH_PAYLOAD);
    pkt_len += fmt_str((char*)pkt.payload, "</lobby>");

    LOGINFO("Sending %s", pkt.payload);

    /* send out the request */
    res = gcoap_req_send(buf, pkt_len, &gs.rd_remote, NULL, NULL, NULL, GCOAP_SOCKET_TYPE_UNDEF);
    if (res <= 0) 
    {
        return CORD_EP_ERR;
    }

    LOGINFO("Done sending lobby resource\n");
    return 0;
}

int unregister_lobby(void)
{
    if(!gs.rd_valid)
    {
        LOGWARN("unregister_lobby called with no valid rd");
        return -1;
    }

    int res;
    ssize_t pkt_len;
    coap_pkt_t pkt;

    /* build and send CoAP POST request to the RD's registration interface */
    res = gcoap_req_init(&pkt, buf, sizeof(buf), COAP_METHOD_POST, get_rd_path());
    if (res < 0) 
    {
        return CORD_EP_ERR;
    }
    /* set some packet options and write query string */
    coap_hdr_set_type(pkt.hdr, COAP_TYPE_CON);
    coap_opt_add_uint(&pkt, COAP_OPT_CONTENT_FORMAT, COAP_FORMAT_LINK);
    res = cord_common_add_qstring(&pkt);
    if (res < 0) 
    {
        return CORD_EP_ERR;
    }

    pkt_len = coap_opt_finish(&pkt, COAP_OPT_FINISH_PAYLOAD);
    pkt_len += fmt_str((char*)pkt.payload, "");

    LOGWARN("Sending %s\n", pkt.payload);

    /* send out the request */
    res = gcoap_req_send(buf, pkt_len, &gs.rd_remote, NULL, NULL, NULL, GCOAP_SOCKET_TYPE_UNDEF);
    if (res <= 0) 
    {
        return CORD_EP_ERR;
    }

    LOGINFO("Done revoking lobby resource\n");
    return 0;
}