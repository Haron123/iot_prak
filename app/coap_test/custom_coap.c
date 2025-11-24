#include "custom_coap.h"

static void saul_to_string(saul_reg_t* saul, int id, char buf[64])
{
    const char* name = saul->name;
    snprintf(buf, 64, "%s: %d", name, id);
}

ssize_t _saul_list(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch (method_flag) 
    {
        case COAP_GET:
            /* Get the list of all Devices*/
            int id = 0;
            saul_reg_t *cur_reg = saul_reg_find_nth(id);

            char buf[256];
            char temp_buf[64];
            
            while(cur_reg)
            {
                saul_to_string(cur_reg, id, temp_buf);
                strcat(buf, temp_buf);
                cur_reg = saul_reg_find_nth(++id);
            }

            /* Init coap response */
            gcoap_resp_init(pdu, (uint8_t*)buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

            /* Add List of devices to coap response */
            resp_len += fmt_str((char*)pdu->payload, buf);

            return resp_len;
    }

    return 0;
}

static void write_payload_to_saul(coap_pkt_t* pdu)
{
    int16_t id = atoi(strtok((char*)pdu->payload, " "));
    int16_t val1 = atoi(strtok(NULL, " "));
    int16_t val2 = atoi(strtok(NULL, " "));
    int16_t val3 = atoi(strtok(NULL, " "));
    phydat_t data = {{val1, val2, val3}, 0, 0};

    saul_reg_write(saul_reg_find_nth(id), &data);
}

ssize_t _saul_action(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch (method_flag) 
    {
        case COAP_GET:
            /* Get the device ID that was asked for*/
            int id = 0;
            if (pdu->payload_len == 1) 
            {
                char payload[] = {0, 0};
                memcpy(payload, (char*)pdu->payload, sizeof(payload));
                id = atoi(payload);
            }

            /* Get Information about device ID as JSON */
            char buf[256];
            read_saul_as_json(id, buf);

            gcoap_resp_init(pdu, (uint8_t*)buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

            resp_len += fmt_str((char*)pdu->payload, buf);
            return resp_len;

        case COAP_PUT:
            write_payload_to_saul(pdu);
            return gcoap_response(NULL, NULL, 0, COAP_CODE_VALID);
    }

    return 0;
}