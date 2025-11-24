#include "boardcontrol.h"

saul_reg_t *setup_led()
{
	return saul_reg_find_type(SAUL_ACT_LED_RGB);
}

saul_reg_t *get_btn()
{
	return saul_reg_find_type(SAUL_SENSE_BTN);
}