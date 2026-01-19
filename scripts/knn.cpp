#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <math.h>
#include <algorithm>

enum SampleAxis
{
	X,
	Y,
	Z,
};

struct Sample
{
	int32_t num;
	int32_t acc_x;
	int32_t acc_y;
	int32_t acc_z;

	void print()
	{
		printf("%d,%d,%d,%d\n", num, acc_x, acc_y, acc_z);
	}

	int32_t get_axis(SampleAxis axis)
	{
		switch(axis)
		{
			case X: return acc_x;
			case Y: return acc_y;
			case Z: return acc_z;
		}

		return X;
	}
};

constexpr uint32_t NUM_FILES = 8;
constexpr uint32_t NUM_SAMPLES = 80;

std::vector<std::string> split(std::string& s, const std::string& delimiter);
static std::vector<Sample> csv_to_arr(const char* file_name)
{
	std::ifstream file(file_name);
	std::vector<Sample> result;

	if(!file.is_open())
	{
		printf("Unable to open file\n");
	}

	int at_line = 0;
	std::string line;
	while(std::getline(file, line))
	{
		if(at_line == 0)
		{
			at_line++;
			continue;
		}
		else
		{
			Sample sample;
			std::vector<std::string> split_str = split(line, ",");
			sample.num   = std::stoi(split_str[0]);
			sample.acc_x = std::stoi(split_str[1]);
			sample.acc_y = std::stoi(split_str[2]);
			sample.acc_z = std::stoi(split_str[3]);
			result.push_back(sample);
			at_line++;
		}
	}

	file.close();
	return result;
}

std::vector<std::string> split(std::string& s, const std::string& delimiter)
{
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
	{
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

struct AccData
{
	private:
	bool is_positive(int32_t num)
	{
		return num > 0;
	}

	public:
	std::array<Sample, NUM_SAMPLES> arr;

	float min(SampleAxis axis)
	{
		float min = INT32_MAX;
		for(int i = 0; i < arr.size(); i++)
		{
			
			if(arr[i].get_axis(axis) < min)
			{
				min = arr[i].get_axis(axis);
			}
		}

		return min;
	}

	float max(SampleAxis axis)
	{
		float max = INT32_MIN;
		for(int i = 0; i < arr.size(); i++)
		{
			if(arr[i].get_axis(axis) > max)
			{
				max = arr[i].get_axis(axis);
			}
		}

		return max;
	}

	float sum(SampleAxis axis)
	{
		float sum = 0;
		for(int i = 0; i < arr.size(); i++)
		{
			sum += arr[i].get_axis(axis);
		}

		return sum;
	}

	bool direction(SampleAxis axis)
	{
		float first_half_sum = 0;
		float second_half_sum = 0;
		for(int i = 0; i < arr.size(); i++)
		{
			if(i < (arr.size() / 2))
			{
				first_half_sum += arr[i].get_axis(axis) - mean(axis);
			}
			else
			{
				second_half_sum += arr[i].get_axis(axis) - mean(axis);
			}
		}

		return ((first_half_sum - second_half_sum)) > 0;
	}

	float mean(SampleAxis axis)
	{
		float sum = 0;
		for(int i = 0; i < arr.size(); i++)
		{
			sum += arr[i].get_axis(axis);
		}

		return sum / arr.size();
	}

	float std(SampleAxis axis)
	{
		float mean = this->mean(axis);
		float sum = 0.0;

		for (int i = 0; i < arr.size(); i++)
		{
			float d = arr[i].get_axis(axis) - mean;
			sum += d * d;
		}

		float variance = sum / arr.size();
		return sqrtf(variance);
	}

	float zcr(SampleAxis axis)
	{
		uint32_t zcr = 0;
		for(int i = 0; i < arr.size(); i++)
		{
			if(i > 0)
			{
				if(is_positive(arr[i].get_axis(axis)) != is_positive(arr[i-1].get_axis(axis)))
				{
					zcr++;
				}
			}
		}

		return (float)zcr;
	}

	float mag()
	{
		float mag = 0;

		for(int i = 0; i < arr.size(); i++)
		{
			Sample &cur = arr[i];
			mag += sqrtf(pow(cur.acc_x, 2) + pow(cur.acc_y, 2) + pow(cur.acc_z, 2));
		}

		return mag;
	}

	float energy(SampleAxis axis)
	{
		float energy = 0;

		for(int i = 0; i < arr.size(); i++)
		{
			energy += arr[i].get_axis(axis) * arr[i].get_axis(axis);
		}

		return energy;
	}

	float rms(SampleAxis axis)
	{
		return sqrtf(this->energy(axis) / (float)arr.size());
	}

	void from_vec(std::vector<Sample> vec)
	{
		for(int i = 0; i < NUM_SAMPLES; i++)
		{
			this->arr[i] = vec[i];
		}
	}
};

float euclidean_distance_between_files(const char* file1, const char* file2);
void print_file_feature_for_array(const char*, const char* file);
int main()
{
	//std::vector<std::pair<int, float>> result;
	
	//std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) 
	//{
    //    return a.second < b.second;
    //});

	//for(int i = 0; i < result.size(); i++)
	//{
	//	printf("Distance to %d is %f\n", result[i].first, result[i].second);
	//}

	print_file_feature_for_array("Serve", "../samples/serve0.csv");
	print_file_feature_for_array("Serve", "../samples/serve1.csv");
	print_file_feature_for_array("Serve", "../samples/serve2.csv");
	print_file_feature_for_array("Serve", "../samples/serve3.csv");
	print_file_feature_for_array("Serve", "../samples/serve4.csv");
	print_file_feature_for_array("Serve", "../samples/serve5.csv");
	print_file_feature_for_array("Serve", "../samples/serve6.csv");
	print_file_feature_for_array("Serve", "../samples/serve7.csv");
	print_file_feature_for_array("Serve", "../samples/serve8.csv");
	print_file_feature_for_array("Serve", "../samples/serve9.csv");

	print_file_feature_for_array("Hit", "../samples/hit0.csv");
	print_file_feature_for_array("Hit", "../samples/hit1.csv");
	print_file_feature_for_array("Hit", "../samples/hit2.csv");
	print_file_feature_for_array("Hit", "../samples/hit3.csv");
	print_file_feature_for_array("Hit", "../samples/hit4.csv");
	print_file_feature_for_array("Hit", "../samples/hit5.csv");
	print_file_feature_for_array("Hit", "../samples/hit6.csv");
	print_file_feature_for_array("Hit", "../samples/hit7.csv");
	print_file_feature_for_array("Hit", "../samples/hit8.csv");
	print_file_feature_for_array("Hit", "../samples/hit9.csv");

	return 0;
}

struct Features 
{
	float min;
	float max;
    float mean;
	float zcr;
	float std;
	float rms;
	bool direction;

	float operator[](int index)
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
		return 6;
	}

	void print()
	{
		printf("min: %f\nmax: %f\nmean: %f\nzcr: %f\nstd: %f\nrms: %f\ndir: %d\n",
		min, max, mean, zcr, std, rms, direction);
	}
};

Features compute_features(AccData data, SampleAxis axis) 
{
    Features f;

	constexpr float max_data = 2000.0;

	f.mean    = data.mean(axis) / max_data;
    f.std     = data.std(axis)  / max_data;
	f.min     = data.min(axis)  / max_data;
	f.max     = data.max(axis)  / max_data;
    f.zcr     = data.zcr(axis)  / (float)data.arr.size();
	f.rms     = data.rms(axis)  / max_data;

	// Dividing by 2 to lower influnce of feature. An influence of 1 would be too big
	f.direction = (float)data.direction(axis) / 2.0f;

    return f;
}

void print_file_feature_for_array(const char* gesture_name, const char* file)
{
	AccData d;
	d.from_vec(csv_to_arr(file));

	std::array<Features, 3> f1;

	f1[0] = compute_features(d, X);
	f1[1] = compute_features(d, Y);
	f1[2] = compute_features(d, Z);

	printf("{\n");
	printf("\tGesture::%s,\n", gesture_name);
	printf("\t{{\n");
	printf("\t\t{ %ff, %ff, %ff, %ff, %ff, %ff, %d },\n", f1[0].min, f1[0].max, f1[0].mean, f1[0].zcr, f1[0].std, f1[0].rms, f1[0].direction);
	printf("\t\t{ %ff, %ff, %ff, %ff, %ff, %ff, %d },\n", f1[1].min, f1[1].max, f1[1].mean, f1[1].zcr, f1[1].std, f1[1].rms, f1[1].direction);
	printf("\t\t{ %ff, %ff, %ff, %ff, %ff, %ff, %d },\n", f1[2].min, f1[2].max, f1[2].mean, f1[2].zcr, f1[2].std, f1[2].rms, f1[2].direction);
	printf("\t}}\n");
	printf("},\n");
}

float euclidean_distance_between_files(const char* file1, const char* file2)
{
	AccData d1;
	AccData d2;

	d1.from_vec(csv_to_arr(file1));
	d2.from_vec(csv_to_arr(file2));

	constexpr uint32_t num_features = Features::len();

	std::array<Features, 3> f1;
	std::array<Features, 3> f2;

	f1[0] = compute_features(d1, X);
	f1[1] = compute_features(d1, Y);
	f1[2] = compute_features(d1, Z);

	f2[0] = compute_features(d2, X);
	f2[1] = compute_features(d2, Y);
	f2[2] = compute_features(d2, Z);

	float distance = 0;
	for(int j = 0; j < 3; j++)
	{
		for(int i = 0; i < num_features; i++)
		{
			float d = (f1[j][i]) - (f2[j][i]);

			distance += d * d;
		}
	}

	return sqrtf(distance);
}