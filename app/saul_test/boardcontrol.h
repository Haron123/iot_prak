#ifndef BOARDCONTROL_H_
#define BOARDCONTROL_H_

#include "saul_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

saul_reg_t *setup_led(void);
saul_reg_t *get_btn(void);

#ifdef __cplusplus
}
#endif

#endif // BOARDCONTROL_H_