#include "gesture_reg.hpp"

static float get_euc_distance(const array<Features, 3> &f1, const array<Features, 3> &f2, float best_so_far)
{
	float distance = 0;
	for(int j = 0; j < 3; j++)
	{
		for(int i = 0; i < Features::len(); i++)
		{
			float d = (f1[j][i]) - (f2[j][i]);

			distance += d * d;

			if(distance > best_so_far)
			{
				return distance;
			}
		}
	}

	float euc_distance = distance;

	return euc_distance;
}

void GestureRecognizer::benchmark()
{
	int16_t data[3] = {10, 200, 300};
	BENCHMARK_FUNC("Time to add imu reading", 100, add_imu_reading(data));

	array<Features, 3> f1;
	BENCHMARK_FUNC("Time to get features", 100, f1[0] = X.get_features());

	f1[0] = X.get_features();
	f1[1] = Y.get_features();
	f1[2] = Z.get_features();
	const array<Features, 3>& f2 = TRAIN_DATA[0].f;

	BENCHMARK_FUNC("Time to get euc distance", 100, get_euc_distance(f1, f2, 1000.0f));

	BENCHMARK_FUNC("Time to add data", 100, 
	X.add_value(data[0]);
	Y.add_value(data[1]);
	Z.add_value(data[2]));
}

Gesture GestureRecognizer::add_imu_reading(int16_t *data)
{
	X.add_value(data[0]);
	Y.add_value(data[1]);
	Z.add_value(data[2]);

	array<Features, 3> f1;
	f1[0] = X.get_features();
	f1[1] = Y.get_features();
	f1[2] = Z.get_features();
	
	array<pair<Gesture, float>, TRAIN_DATA.size()> results;
	float min = 10000.0f;
	uint32_t min_index = 0;
	for(unsigned int i = 0; i < TRAIN_DATA.size(); i++)
	{
		const array<Features, 3>& f2 = TRAIN_DATA[i].f;
		results[i] = {TRAIN_DATA[i].gesture, get_euc_distance(f1, f2, min)};

		if(results[i].second < min)
		{
			min = results[i].second;
			min_index = i;
		}
	}

	if(min > 0.7f * 0.7f)
	{
		return Gesture::None;
	}
	else
	{
		return results[min_index].first;
	}
}

void GestureRecognizer::print_current_features()
{
	array<Features, 3> f1;
	f1[0] = X.get_features();
	f1[1] = Y.get_features();
	f1[2] = Z.get_features();

	for(int j = 0; j < 3; j++)
	{
		printf("%d\n", j);
		f1[j].print();
	}
}

void GestureRecognizer::print_distance_to(uint16_t training_data_index)
{
	array<Features, 3> f1;
	f1[0] = X.get_features();
	f1[1] = Y.get_features();
	f1[2] = Z.get_features();

	array<Features, 3> f2 = TRAIN_DATA[training_data_index].f;
	
	printf("Distance to %d: %f\n", training_data_index, get_euc_distance(f1, f2, 1000.0f));
}

uint32_t GestureRecognizer::get_strength()
{
	uint32_t result = 0;

	result += sqrtf(X.get_energy());
	result += sqrtf(Y.get_energy());
	result += sqrtf(Z.get_energy());

	return result;
}