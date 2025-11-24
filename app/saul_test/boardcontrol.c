#include "boardcontrol.h"

saul_reg_t *setup_led(void)
{
	return saul_reg_find_type(SAUL_ACT_LED_RGB);
}

saul_reg_t *get_btn(void)
{
	return saul_reg_find_type(SAUL_SENSE_BTN);
}