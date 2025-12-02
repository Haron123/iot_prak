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

#include "net/gcoap.h"
#include "shell.h"

#include "custom_coap.h"
#include "boardcontrol.h"
#include "gcoap_example.h"

#define MAIN_QUEUE_SIZE (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static int print_saul_cmd_help(void)
{
    printf("Usage: saul\n");
    printf("help                          \t Prints this\n");
    printf("list                          \t Lists all SAUL Devices available\n");
    printf("read id                       \t Reads the SAUL Device at ID\n");
    printf("write id value1 value2 value3 \t Writes to the SAUL Device at ID\n");

    return 0;
}

static int saul_cmd(int argc, char** argv)
{
    printf("-------------------------------\n");
    if(argc == 2 && !strcmp(argv[1], "list"))
    {
        list_saul();
    }
    else if (argc == 2 && !strcmp(argv[1], "help"))
    {
        print_saul_cmd_help();
    }
    else if(argc == 3)
    {
        if(!strcmp(argv[1], "read"))
        {
            print_saul_device(atoi(argv[2]));
        }
        else
        {
            print_saul_cmd_help();
        }
    }
    else if(argc == 6 && !strcmp(argv[1], "write"))
    {
        int16_t id   = atoi(argv[2]);
        int16_t val1 = atoi(argv[3]);
        int16_t val2 = atoi(argv[4]);
        int16_t val3 = atoi(argv[5]);

        phydat_t data = {{val1, val2, val3}, 0, 0};

        saul_reg_t *saul = saul_reg_find_nth(id);

        int err = saul_reg_write(saul, &data);

        switch(err)
        {
            case -ENODEV:
                printf("Device %d does not exist\n", id);
                break;
            case -ENOTSUP:
                printf("Device %d does not support this operation\n", id);
                break;
            case -ECANCELED:
                printf("Device %d had an error\n", id);
                break;
            default:
                printf("Successfully written\n");
        }
    }
    else
    {
        print_saul_cmd_help();
    }
    printf("-------------------------------\n");

    return 1;
}

static int test_cmd(int argc, char** argv)
{
    set_led();

    return 1;
}

int main(void)
{
    /* for the thread running the shell */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    server_init();
    puts("gcoap example app");

    /* Shell commands */
    static const shell_command_t shell_commands[] = 
    {
        {"saul", "Interface with SAUL devices", saul_cmd},
        {"test", "test", test_cmd},
        { NULL, NULL, NULL },
    };

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}
