#include "imu.hpp"

IMU::IMU(): acc_saul(NULL), gyro_saul(NULL), mag_saul(NULL)
{

}

void IMU::init()
{
	acc_saul = saul_reg_find_type(SAUL_SENSE_ACCEL);
	gyro_saul = saul_reg_find_type(SAUL_SENSE_GYRO);
	mag_saul = saul_reg_find_type(SAUL_SENSE_MAG);
}

phydat_t IMU::read_acc()
{
	int dimension = saul_reg_read(acc_saul, &temp_data);
    if(dimension <= 0) {printf("Error reading ACC device\n");}

	return temp_data;
}

phydat_t IMU::read_gyro()
{
	int dimension = saul_reg_read(gyro_saul, &temp_data);
    if(dimension <= 0) {printf("Error reading GYRO device\n");}

	return temp_data;
}

phydat_t IMU::read_mag()
{
	int dimension = saul_reg_read(mag_saul, &temp_data);
    if(dimension <= 0) {printf("Error reading MAG device\n");}

	return temp_data;
}
