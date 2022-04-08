#include <cstdint>
void Labeling_Null(unsigned* dest, const uint8_t* source, int height, int width, int data_width, int fmbits) {
	//just copy
	for (int i = 0; i < height; i++) {
		const uint8_t* datas = source + data_width * i;
		unsigned* labels = dest + width * i;
		for (int j = 0; j < width; j++) {
			labels[j] = datas[j];
		}
	}
}