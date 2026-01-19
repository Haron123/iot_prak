#ifndef GESTURE_REG_H_
#define GESTURE_REG_H_

#include <stdint.h>
#include <array>
#include <math.h>
#include <stdio.h>
#include "gesture.hpp"
#include "benchmark.h"
#include "training_data.hpp"

using namespace std;

class GestureRecognizer
{
private:
	Signal X;
	Signal Y;
	Signal Z;

public:
	void benchmark();
	Gesture add_imu_reading(int16_t* data);
	void print_current_features();
	void print_distance_to(uint16_t training_data_index);
	uint32_t get_strength();
};

#endif // GESTURE_REG_H_