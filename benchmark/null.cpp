#include "Common.h"
void null_algorithm(const Data& data, Data& data_labels) {
	int height = data.height;
	int width = data.width;

	//just copy
	for (int i = 0; i < height; i++) {
		const unsigned int* datas = data.data[i];
		unsigned int* labels = data_labels.data[i];
		for (int j = 0; j < width; j++) {
			labels[j] = datas[j];
		}
	}
}