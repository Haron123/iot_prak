#ifndef GESTURE_H_
#define GESTURE_H_

#include <stdint.h>
#include <array>
#include <math.h>
#include <stdio.h>
#include "benchmark.h"

constexpr uint32_t NUM_SAMPLES = 80;

static bool is_positive(int32_t num)
{
	return num >= 0;
}

struct Features 
{
	float min;
	float max;
    float mean;
	float zcr;
	float std;
	float rms;
	float direction;

	float operator[](int index) const
	{
		switch(index)
		{
			case 0: return min;
			case 1: return max;
			case 2: return mean;
			case 3: return zcr;
			case 4: return std;
			case 5: return rms;
			case 6: return direction;
			default: return 0.0f;
		}
	}

	static constexpr int len()
	{
		return 7;
	}

	void print()
	{
		printf("min: %f\nmax: %f\nmean: %f\nzcr: %f\nstd: %f\nrms: %f\ndir: %f\n",
		min, max, mean, zcr, std, rms, direction);
	}
};

class Signal
{
private:
	uint32_t read_head  = 0;
	uint32_t write_head = 0;
	uint32_t sample_count  = 0;
	std::array<int32_t, NUM_SAMPLES> samples;

	int32_t last_deleted_sample = 0;
	int32_t min = INT32_MAX;
	int32_t max = INT32_MIN;
	int64_t sum = 0;
	bool direction = false;
	float mean  = 0;
	float non_sqrt_std = 0;
	float std   = 0;
	int32_t zcr_count   = 0;
	float energy = 0;
	float power = 0;
	float rms   = 0;

	bool is_deleting()
	{
		return sample_count >= NUM_SAMPLES;
	}

	int32_t get_oldest_sample()
	{
		return samples[read_head];
	}

	int32_t get_deleted_sample()
	{
		return last_deleted_sample;
	}

	int32_t get_newest_sample()
	{
		return samples[(write_head + NUM_SAMPLES - 1) % NUM_SAMPLES];
	}

	int32_t get_second_newest_sample()
	{
		return samples[(write_head + NUM_SAMPLES - 2) % NUM_SAMPLES];
	}

	void update_min_max_direction_std()
	{
		min = INT32_MAX;
		max = INT32_MIN;

		float first_half_sum = 0;
		float second_half_sum = 0;
		float sum = 0.0;
		for(uint32_t i = 0; i < sample_count; i++)
		{
			uint32_t index = (read_head + i) % NUM_SAMPLES;
			if(samples[index] < min)
			{
				min = samples[index];
			}

			if(samples[index] > max)
			{
				max = samples[index];
			}

			if(i < (sample_count / 2))
			{
				first_half_sum += samples[index] - mean;
			}
			else
			{
				second_half_sum += samples[index] - mean;
			}

			float d = samples[index] - mean;
			sum += d * d;
		}

		direction = ((first_half_sum - second_half_sum)) > 0;

		float variance = sum / sample_count;
		std = sqrtf(variance);
	}
	
	void update_sum()
	{
		if(is_deleting())
		{
			sum -= get_deleted_sample();
		}
		sum += get_newest_sample();
	}

	void update_mean()
	{
		mean = (float)sum / (float)sample_count;
	}

	void update_zcr_count()
	{
		if(sample_count > 1)
		{
			int32_t newest = get_newest_sample();
			int32_t second_newest = get_second_newest_sample();
			if(is_positive(newest) != is_positive(second_newest))
			{
				zcr_count++;
			}

			if(is_deleting())
			{
				int32_t deleted = get_deleted_sample();
				int32_t oldest  = get_oldest_sample();
				if(is_positive(deleted) != is_positive(oldest))
				{
					zcr_count--;
				}
			}
		}
	}

	void update_energy()
	{
		if(is_deleting())
		{
			energy -= (float)get_deleted_sample() * (float)get_deleted_sample();
		}

		energy += (float)get_newest_sample() * (float)get_newest_sample();
	}
	
	void update_power_and_rms()
	{
		power = energy / (float)sample_count;
		rms = sqrtf(power);
	}

public:
	float get_energy()
	{
		return energy;
	}

	Features get_features()
	{
		Features f;	

		constexpr float max_data = 2000.0;

		f.mean    = (float)mean / max_data;
		f.std     = (float)std  / max_data;
		f.min     = (float)min  / max_data;
		f.max     = (float)max  / max_data;
		f.zcr     = (float)zcr_count  / (float)sample_count;
		f.rms     = (float)rms  / max_data;
		f.direction = direction;

		return f;
	}

	void benchmark()
	{
		BENCHMARK_FUNC("Time to calculate all Features", 100, add_value(10));
		BENCHMARK_FUNC("Time spent in update_min_max_direction_std", 100, update_min_max_direction_std());
		BENCHMARK_FUNC("Time spent in update_sum", 100, update_sum());
		BENCHMARK_FUNC("Time spent in update_mean", 100, update_mean());
		BENCHMARK_FUNC("Time spent in update_zcr_count", 100, update_zcr_count());
		BENCHMARK_FUNC("Time spent in update_energy", 100, update_energy());
		BENCHMARK_FUNC("Time spent in update_power_and_rms", 100, update_power_and_rms());
	}

	void add_value(int32_t new_sample)
	{
		if(sample_count >= NUM_SAMPLES)
		{
			last_deleted_sample = samples[read_head];
			read_head = (read_head + 1) % NUM_SAMPLES;
		}
		else
		{
			sample_count++;
		}

		samples[write_head] = new_sample;
		write_head = (write_head + 1) % NUM_SAMPLES;

		/* Order of Updates is relevant, as some use values of the other */
		update_sum();
		update_mean();
		update_min_max_direction_std();
		update_zcr_count();
		update_energy();
		update_power_and_rms();
	}
};

enum Gesture
{
	None,
	Serve,
	Hit,
};

struct training_data
{
	Gesture gesture;
	std::array<Features, 3> f;
};

#endif // GESTURE_H_