#ifndef BOARDCONTROL_H_
#define BOARDCONTROL_H_

#include "saul_reg.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int get_button(void);

void print_saul_device(int id);

void read_saul_as_json(int id, char *buf);

void list_saul(void);

#ifdef __cplusplus
}
#endif

#endif // BOARDCONTROL_H_