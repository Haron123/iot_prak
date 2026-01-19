#ifndef MATH_UTIL_H_
#define MATH_UTIL_H_

#define _USE_MATH_DEFINES
#include <cmath>

#define M_PI 3.14159265358979323846	/* pi */

struct EulerAngles
{
	float roll, pitch, yaw;
};

static float QuaternionToRoll(float qw, float qx, float qy, float qz)
{
	float sinr_cosp = 2*(qw*qx + qy*qz);
	float cosr_cosp = 1-(qx*qx + qy*qy);
	return std::atan2(sinr_cosp, cosr_cosp);
}

static float QuaternionToPitch(float qw, float qx, float qy, float qz)
{
	float sinp = std::sqrt(1+2*(qw*qy - qx*qz));
    float cosp = std::sqrt(1-2*(qw*qy - qx*qz));
    return 2 * std::atan2(sinp, cosp) - M_PI / 2;
}

static float QuaternionToYaw(float qw, float qx, float qy, float qz)
{
	float siny_cosp = 2*(qw*qz + qx*qy);
    float cosy_cosp = 1-2*(qy*qy + qz*qz);
    return std::atan2(siny_cosp, cosy_cosp);
}

static EulerAngles QuaternionToEuler(float qw, float qx, float qy, float qz)
{
	EulerAngles e;

	e.roll = QuaternionToRoll(qw, qx, qy, qz);
	e.pitch = QuaternionToPitch(qw, qx, qy, qz);
	e.yaw = QuaternionToYaw(qw, qx, qy, qz);

	return e;
}

#endif // MATH_UTIL_H_