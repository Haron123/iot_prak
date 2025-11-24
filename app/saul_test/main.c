#include <stdio.h>
#include "shell.h"

#include "boardcontrol.h"

static int say_hello(int argc, char **argv)
{
    puts("Hello\n");

    return 1;
}

static int get_button(int argc, char** argv)
{
    saul_reg_t *btn = get_btn();

    if(btn)
    {
        phydat_t btn_state;
        saul_reg_read(btn, &btn_state);

        printf("Button found: %s\n", btn->name);
        printf("Value of button: %u\n", btn_state.val[0]);
    }

    return 1;
}

int main(void)
{
    puts("Welcome to RIOT!\n");
    puts("Type `help` for help, type `saul` to see all SAUL devices\n");

    static const shell_command_t shell_commands[] = 
    {
        {"hello", "Prints Hello to the console", say_hello},
        {"read_btn", "Reads a random button", get_button},
        { NULL, NULL, NULL },
    };

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}

