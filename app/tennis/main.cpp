/*
 * SPDX-FileCopyrightText: 2015-2016 Ken Bannister
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       gcoap example
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 *
 * @}
 */

#include <stdio.h>
#include "msg.h"

#include "ztimer.h"
#include "coap_client.h"
#include "net/gcoap.h"
#include "shell.h"
#include "gcoap_example.h"
#include "boardcontrol.h"
#include "imu.hpp"

#include "net/ipv6/addr.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/netif.h"
#include "net/netopt.h"
#include "net/ipv6.h"
#include "logging.h"
#include "gesture_reg.hpp"
#include "MadgwickAHRS.h"
#include "MahonyAHRS.h"
#include "math_util.hpp"
#include "saul.h"
#include "bmx280.h"
#include "bmx280_params.h"
#include "net/gnrc/netif/pktq.h"
#include "net/ieee802154.h"
#include "net/netdev.h"
#include "net/netdev/ieee802154.h"

#define MAIN_QUEUE_SIZE (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static int test_cmd(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    
    char* uri = get_rd_path();
    sock_udp_ep_t* remote = get_rd_remote();

    printf("%s\n", uri);
    for(unsigned int i = 0; i < sizeof(remote->addr.ipv6); i++)
    {
        printf("%02X", remote->addr.ipv6[i]);
    }
    printf(":%X\n", remote->port);

    return 1;
}

static int reg(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    register_to_rd();

    return 1;
}

static int print_sauls(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    list_saul();

    return 1;
}

static int print_id(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    #ifdef MYID
        printf("This is node %d\n", MYID);
    #else
        printf("No ID given. Define MYID");
    #endif

    return 1;
}

static int get_resources(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    update_resources();

    return 1;
}

static int find_playerss(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    find_players();

    return 1;
}

#ifndef NETIF_PRINT_IPV6_NUMOF
#define NETIF_PRINT_IPV6_NUMOF 4
#endif

static int print_ip(int argc, char** argv)
{
    (void)argc;
    (void)argv;

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
                    ipv6_addr_print(addrs + i);
                    puts("\n");
                }
            }
        }
    }

    return 1;
}

static int post_hitt(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    post_hit(100);

    return 1;
}

char shell_thread_stack[THREAD_STACKSIZE_MAIN];
static void *shell_thread(void *arg)
{
    (void)arg;

    (void)_main_msg_queue;
    (void)shell_thread;
    /* for the thread running the shell */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    server_init();
    puts("gcoap example app");

    /* Shell commands */
    static const shell_command_t shell_commands[] = 
    {
        {"test", "test", test_cmd},
        {"reg", "reg", reg},
        {"list_sauls", "Lists all Saul devices", print_sauls},
        {"get_id", "Gives ID of this device", print_id},
        {"update_resources", "Updates resource list", get_resources},
        {"find_players", "Finds players from resource list", find_playerss},
        {"print_ips", "prints ips", print_ip},
        {"hit", "hit happend", post_hitt},
        { NULL, NULL, NULL },
    };

    /* start shell */
    LOGINFO("Starting Shell");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}

static void wait_for_button(void)
{
    saul_reg_t* button = saul_reg_find_name("UserSw");
    phydat_t button_data;

    do
    {
        saul_reg_read(button, &button_data);
        ztimer_sleep(ZTIMER_MSEC, 10);
    }
    while(!button_data.val[0]);
}

static bool button_is_pressed(void)
{
    saul_reg_t* button = saul_reg_find_name("UserSw");
    phydat_t button_data;

    saul_reg_read(button, &button_data);
    return button_data.val[0];
}

#define NUM_SAMPLES 80
static int16_t measurements[NUM_SAMPLES][3] = {0};
static void sample_loop(void)
{
    while(1)
    {
        wait_for_button();
        
        IMU imu;
        imu.init();
        printf("Num,Acc_X,Acc_Y,Acc_Z\n");
        for(int i = 0; i < NUM_SAMPLES; i++)
        {
            ztimer_sleep(ZTIMER_MSEC, 5);
            phydat_t acc = imu.read_acc();

            memcpy(measurements[i], acc.val, sizeof(measurements[0]));
        }

        for(int i = 0; i < NUM_SAMPLES; i++)
        {
            printf("%d,%d,%d,%d\n", i, measurements[i][0], measurements[i][1], measurements[i][2]); 
        }


        printf("EOF\n");
    }
}

static GestureRecognizer rec;
static void test_loop(void)
{
    uint32_t skip = 0;
    while(1)
    {
        IMU imu;
        imu.init();
        wait_for_button();
        printf("Started\n");   
        
        for(int i = 0; i < NUM_SAMPLES*1000; i++)
        {
            ztimer_sleep(ZTIMER_MSEC, 5);
            phydat_t acc = imu.read_acc();
            Gesture g = rec.add_imu_reading(acc.val);
            
            if(skip > 0)
            {
                skip--;
                continue;
            }

            if(g != Gesture::None)
            {
                //for(unsigned int i = 0; i < TRAIN_DATA.size(); i++)
                //{
                //    rec.print_distance_to(i);
                //}
                //rec.print_current_features();
            }
            
            if(g == Gesture::Hit)
            {
                printf("Hit\n");
                skip = 100;
            }
            if(g == Gesture::Serve)
            {
                printf("Serve\n");
                skip = 100;
            }
        }
    }
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

struct __attribute__((packed)) QandHeight
{
    float q0;
    float q1;
    float q2;
    float q3;
    uint8_t id;
    char lobby_id[6];
    int32_t height;
};

static char* gui_uri;
static bool uri_set = false;
static int32_t height = 0;
static uint8_t str2remote_buf[128];
static void send_q_and_height(void)
{
    if(!uri_set)
    {
        if(find_hit_uri() < 0)
        {
            printf("No Gui URI Found...\n");
            return;
        }
        else
        {
            gui_uri = gs.lobby_arr[find_hit_uri()];
            uri_set = true;
        }
    }
    
    QandHeight q;
    q.q0 = q0;
    q.q1 = q1;
    q.q2 = q2;
    q.q3 = q3;
    q.id = gs.is_lobby;
    strcpy(q.lobby_id, gs.lobby_id);
    q.height = height;
    //coap_send_buf(gui_uri, (uint8_t*)&q, sizeof(QandHeight), COAP_POST, COAP_TYPE_NON);

    int res = 0;
    if(gnrc_netif_pktq_usage() < CONFIG_GNRC_NETIF_PKTQ_POOL_SIZE / 2)
    {
        sock_udp_ep_t sock;
        _uristr2remote(gui_uri, &sock, NULL, (char*)str2remote_buf, sizeof(str2remote_buf));
        res = sock_udp_send(NULL, (uint8_t*)&q, sizeof(QandHeight), &sock);
    }
    else
    {
        LOGINFO("Packet queue too full");
    }

    if(res < 0 || dropping_packet)
    {
        // When theres an error, wait 
        LOGINFO("Packet has been dropped");
        ztimer_sleep(ZTIMER_MSEC, 1000);
        dropping_packet = false;
    }
}

#define M_PI 3.14159265358979323846	/* pi */
static phydat_t gyro_drift;
static void update_q(IMU& imu)
{
    float gx_drift = (float)gyro_drift.val[0];
    float gy_drift = (float)gyro_drift.val[1];
    float gz_drift = (float)gyro_drift.val[2];

    phydat_t acc = imu.read_acc();
    phydat_t gyro = imu.read_gyro();
    phydat_t mag = imu.read_mag();

    float gx, gy, gz, ax, ay, az, mx, my, mz = 0.0f;
    ax = ((float)acc.val[0]) / 1000.0f;
    ay = ((float)acc.val[1]) / 1000.0f;
    az = ((float)acc.val[2]) / 1000.0f;

    gx = ((float)gyro.val[0] - gx_drift) * (M_PI / 180.0f);
    gy = ((float)gyro.val[1] - gy_drift) * (M_PI / 180.0f);
    gz = ((float)gyro.val[2] - gz_drift) * (M_PI / 180.0f);

    mx = ((float)mag.val[0]) / 1000.0f;
    my = ((float)mag.val[1]) / 1000.0f;
    mz = ((float)mag.val[2]) / 1000.0f;

    //MadgwickAHRSupdate(gx, gy, gz, ax, ay, az, mx, my, mz);
    (void)QuaternionToEuler;
    MahonyAHRSupdate(gx, gy, gz, ax, ay, az, mx, my, mz);
}

Gesture get_gesture(IMU& imu)
{
    static uint32_t skip = 0;

    phydat_t acc = imu.read_acc();
    Gesture g = rec.add_imu_reading(acc.val);
    
    if(skip > 0)
    {
        skip--;
        return Gesture::None;
    }

    if(g != Gesture::None) 
    {
        skip = 100;
    }
   
    return g;
}

static bmx280_t dev;
static uint32_t time_passed = 0;
static uint32_t last_pressure = 0;
void game_loop(IMU& imu)
{
    ztimer_sleep(ZTIMER_MSEC, 10);
    update_q(imu);
    uint32_t pressure = bmx280_read_pressure(&dev);
    height += (last_pressure - pressure);
    last_pressure = pressure;

    time_passed += 10;
    gs.heartbeat_timer += 10;
    if(time_passed >= 50)
    {
        send_q_and_height();
        time_passed = 0;
    }
    
    if(button_is_pressed())
    {
        height = 0;
    }

    //if(gs.heartbeat_timer == 5000)
    //{
    //    LOGINFO("Sending heartbeat");
    //    heartbeat(gs.heartbeat_uri);
    //}
    //if(gs.heartbeat_timer > 10000)
    //{
    //    gs.in_game = false;
    //}
}

void write_led(saul_reg_t* rgb, uint8_t r, uint8_t g, uint8_t b)
{
    phydat_t data;
    data.val[0] = r;
    data.val[1] = g;
    data.val[2] = b;

    saul_reg_write(rgb, &data);
}

int main(void)
{
    ztimer_sleep(ZTIMER_MSEC, 1000);
    bmx280_init(&dev, &bmx280_params[0]);
    ztimer_sleep(ZTIMER_MSEC, 4000);
    thread_create(shell_thread_stack, sizeof(shell_thread_stack), THREAD_PRIORITY_MIN, 0, shell_thread, NULL, "Shell thread");

    saul_reg_t* rgb = saul_reg_find_type(SAUL_ACT_LED_RGB);
    write_led(rgb, 30, 30, 30);

    /* Find the RD at the start */
    while(update_rd() < 0)
    {
        LOGINFO("Searching Resource directory...");
        ztimer_sleep(ZTIMER_MSEC, 1000);
        write_led(rgb, 0, 0, 10);
    }

    (void)test_loop;
    (void)sample_loop;

    IMU imu;
    imu.init();
    gyro_drift = imu.read_gyro();
    while(1)
    {
start:
        wait_for_button();
        while(find_hit_uri() < 0)
        {
            LOGINFO("Searching for GUI...");
            update_resources();
            ztimer_sleep(ZTIMER_MSEC, 1000);
            write_led(rgb, 10, 0, 10);
        }
        write_led(rgb, 10, 10, 0);
        LOGINFO("Starting search for players...");
        int result = find_players();

        last_pressure = bmx280_read_pressure(&dev);
        if(result == -1)
        {
            LOGINFO("No other players found, starting new lobby");
            while(register_lobby() < 0)
            {
                LOGINFO("Registering Lobby...");
                ztimer_sleep(ZTIMER_MSEC, 1000);
            }
            gs.is_lobby = true;
            while(1)
            {
                ztimer_sleep(ZTIMER_MSEC, 100);
                if(gs.in_game)
                {
                    write_led(rgb, 0, 10, 0);
                    ztimer_sleep(ZTIMER_MSEC, 100);

                    while(1)
                    {
                        game_loop(imu);

                        if(!gs.in_game)
                        {
                            LOGINFO("Partner failed heartbeat");
                            end_duel(gs.partner_end_duel_uri);
                            write_led(rgb, 10, 10, 0);
                            goto start;
                        }
                    }
                }
            }
        }
        else
        {
            gs.is_lobby = false;
            LOGINFO("Found player %d", result);
        }

        if(result >= 0)
        {
            ztimer_sleep(ZTIMER_MSEC, 100);
            LOGINFO("Starting a duel");
            build_partner_uris(gs.lobby_arr[result]);
            bool accepted = start_duel(gs.partner_start_duel_uri);
            
            if(accepted)
            {
                printf("Accepted into Lobby with ID %s\n", get_lobby_id());
                if(gs.in_game)
                {
                    write_led(rgb, 0, 10, 0);
                    while(1)
                    {
                        game_loop(imu);

                        if(!gs.in_game)
                        {
                            LOGINFO("Partner failed heartbeat");
                            end_duel(gs.partner_end_duel_uri);
                            write_led(rgb, 10, 10, 0);
                            goto start;
                        }
                    }
                }
            }
        }

        ztimer_sleep(ZTIMER_MSEC, 1000);
    }

    return 0;
}
