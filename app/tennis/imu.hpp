#ifndef IMU_H_
#define IMU_H_

#include "saul_reg.h"
#include <stdio.h>

class IMU
{
private:
	saul_reg_t *acc_saul;
	saul_reg_t *gyro_saul;
	saul_reg_t *mag_saul;
	phydat_t temp_data;

public:
	IMU();
	void init();
	phydat_t read_acc();
	phydat_t read_gyro();
	phydat_t read_mag();
};

#endif // IMU_H_