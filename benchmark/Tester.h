#ifndef HEADER_TESTER
#define HEADER_TESTER
#include <utility>
class LabelMap {
public:
	unsigned int* labels;
	int height;
	int width;

public:
	LabelMap() : labels(nullptr), height(0), width(0) {};
	LabelMap(unsigned int* _labels, int _height, int _width) : labels(_labels), height(_height), width(_width) {};
	~LabelMap() {
		if (labels) delete[] labels;
	}

public:
	bool is_equiv(LabelMap other);
	bool export_as(const char* filename);
};

class Bit_Source;
class Byte_Source {
public:
	uint8_t* data;
	int height;
	int width;
	int data_width;

public:
	Byte_Source() : data(nullptr), height(0), width(0), data_width(0) {};
	Byte_Source(uint8_t* _data, int _height, int _width, int _data_width) : data(_data), height(_height), width(_width), data_width(_data_width) {};

public:
	void Initialize(Bit_Source& bit_source);
	void Initialize_by_Rand(int _height, int _width);
	bool Initialize_by_File(const char* filename);
	void Release() {
		if (data) delete[] data;
	}
};
class Bit_Source {
public:
	void* data;
	int height;
	int width;
	int data_width;
	int fmbits;

public:
	Bit_Source() : data(nullptr), height(0), width(0), data_width(0), fmbits(0) {};
	Bit_Source(unsigned int* _data, int _height, int _width, int _data_width, int _fmbits)
		: data(_data), height(_height), width(_width), data_width(_data_width), fmbits(_fmbits) {};

public:
	void Initialize(Byte_Source& byte_source, int _data_width, int _fmbits);
	void Initialize_by_Rand(int _height, int _width, int _data_width, int _fmbits);
	bool Initialize_by_File(const char* filename);
	void Release() {
		if (data) delete[] data;
	}
};
typedef void (*Byte_Algorithm) (unsigned int* dest, const uint8_t* source, int height, int width, int data_width, int fmbits);
typedef void (*Bit_Algorithm) (unsigned int* dest, const void* source, int height, int width, int data_width, int fmbits);

LabelMap PerformLabeling(Byte_Algorithm byte_algorithm, Byte_Source& byte_source);
LabelMap PerformLabeling(Bit_Algorithm bit_algorithm, Bit_Source& bit_source);

bool Test_Correctness(Byte_Algorithm byte_algorithm, Byte_Source& byte_source, Bit_Algorithm bit_algoritm, Bit_Source& bit_source);
bool Test_Correctness(Byte_Algorithm byte_algorithm, Byte_Source& byte_source, Byte_Algorithm byte_algoritm_t, Byte_Source& byte_source_t);
int Test_Performance(Byte_Algorithm byte_algorithm, Byte_Source& byte_source, int count);
int Test_Performance(Bit_Algorithm bit_algorithm, Bit_Source& bit_source, int count);
#endif