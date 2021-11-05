#include "Label_Solver.h"
inline void CCL_TS4_FirstPass(unsigned* dest, int height, int width, UFPC& labelsolver) {
	if (width == 0 || height == 0) return;

	unsigned int* labels = dest;
	if (labels[0]) labels[0] = labelsolver.NewLabel();
	for (int j = 1; j < width; j++) {
		if (!labels[j]) continue;
		if (labels[j - 1])  labels[j] = labels[j - 1];
		else labels[j] = labelsolver.NewLabel();
	}

	for (int row = 1; row < height; row++) {
		unsigned int* labels = dest + row * width;
		unsigned int* labels_up = dest + (row - 1) * width;
		if (labels[0]) {
			if (labels_up[0]) labels[0] = labels_up[0];
			else labels[0] = labelsolver.NewLabel();
		}
		for (int j = 1; j < width; j++) {
			if (!labels[j]) continue;
			if (labels_up[j]) {
				if (!labels[j - 1] || labels[j - 1] == labels_up[j]) labels[j] = labels_up[j];
				else {
					unsigned label = labelsolver.GetLabel(labels_up[j]);
					unsigned label_other = labelsolver.GetLabel(labels[j - 1]);
					if (label != label_other) label = labelsolver.Merge(label, label_other);
					labels[j] = label;
				}
			}
			else if (labels[j - 1]) labels[j] = labels[j - 1];
			else labels[j] = labelsolver.NewLabel();
		}
	}
	labelsolver.Flatten();
}
void Labeling_TS4(unsigned* dest, const unsigned int* source, int height, int width) {
	//initialize label state
	for (int i = 0; i < height; i++) {
		const unsigned int* datas = source + i * width;
		unsigned int* labels = dest + i * width;
		for (int j = 0; j < width; j++) {
			if (datas[j]) labels[j] = 1;
			else labels[j] = 0;
		}
	}

	//first pass
	UFPC labelsolver;
	labelsolver.Alloc(((height + 1) / 2) * ((width + 1) / 2) + 1);
	labelsolver.Setup();
	CCL_TS4_FirstPass(dest, height, width, labelsolver);

	//second pass
	for (int i = 0; i < height; i++) {
		unsigned int* labels = dest + i * width;
		for (int j = 0; j < width; j++) {
			if (labels[j]) labels[j] = labelsolver.GetLabel(labels[j]);
		}
	}

	//release memory 
	labelsolver.Dealloc();
}