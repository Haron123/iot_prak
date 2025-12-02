#include "boardcontrol.h"

int get_button(void)
{
	saul_reg_t *btn =  saul_reg_find_type(SAUL_SENSE_BTN);
    phydat_t btn_state;

    if(btn)
    {
        saul_reg_read(btn, &btn_state);
        return btn_state.val[0];
    }

    return -1;
}

void print_saul_device(int id)
{
    saul_reg_t *saul = saul_reg_find_nth(id);

    if(!saul) {printf("no Device with ID %d found\n", id); return;}

    phydat_t data;
    int dimension = saul_reg_read(saul, &data);

    if(dimension <= 0) {printf("Error reading Device with ID %d\n", id); return;}

    phydat_dump(&data, dimension);
}

void read_saul_as_json(int id, char* buf)
{
    saul_reg_t *saul = saul_reg_find_nth(id);

    if(!saul) {printf("no Device with ID %d found\n", id); return;}

    phydat_t data;
    int dimension = saul_reg_read(saul, &data);

    if(dimension <= 0) {printf("Error reading Device with ID %d\n", id); return;}

    phydat_dump(&data, dimension);
    phydat_to_json(&data, dimension, buf);
}

void list_saul(void)
{
    int cur = 0;
    saul_reg_t *cur_reg = saul_reg_find_nth(cur);

    printf("--------------------------\n");
    printf("Found Devices:\n");
    while(cur_reg)
    {
        printf("%d: %s \n", cur, cur_reg->name);
        cur++;
        cur_reg = saul_reg_find_nth(cur);
    }
    printf("--------------------------\n");
}