void Labeling_Null(unsigned* dest, const unsigned int* source, int height, int width) {
	//just copy
	for (int i = 0; i < height; i++) {
		const unsigned int* datas = source + width * i;
		unsigned* labels = dest + width * i;
		for (int j = 0; j < width; j++) {
			labels[j] = datas[j];
		}
	}
}