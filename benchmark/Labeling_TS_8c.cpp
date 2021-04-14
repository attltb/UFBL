#include "Label_Solver.h"
inline void CCL_TS8_FirstPass(unsigned* dest, int height, int width, UFPC& labelsolver) {
	if (width == 0 || height == 0) return;

	unsigned int* labels = dest;
	if (labels[0]) labels[0] = labelsolver.NewLabel();
	for (int j = 1; j < width; j++) {
		if (!labels[j]) continue;
		if (labels[j - 1])  labels[j] = labels[j - 1];
		else labels[j] = labelsolver.NewLabel();
	}

	for (int row = 1; row < height; row++) {
		unsigned int* labels = dest + (size_t)row * width;
		unsigned int* labels_up = dest + ((size_t)row - 1) * width;

		if (width == 1) {
			if (labels[0]) {
				if (labels_up[0]) labels[0] = labels_up[0];
				else labels[0] = labelsolver.NewLabel();
			}
			continue;
		}
		if (width == 2) {
			unsigned int label = 0;
			if (labels_up[0]) label = labels_up[0];
			else if (labels_up[1]) label = labels_up[1];
			if (labels[0]) {
				if (label) labels[0] = label;
				else labels[0] = labelsolver.NewLabel();
				if (labels[1]) labels[1] = labels[0];
			}
			else if (labels[1]) {
				if (label) labels[1] = label;
				else labels[1] = labelsolver.NewLabel();
			}
			continue;
		}

		if (labels[0]) {
			if (labels_up[0]) labels[0] = labels_up[0];
			else if (labels_up[1]) labels[0] = labels_up[1];
			else labels[0] = labelsolver.NewLabel();
		}
		for (int j = 1; j < width - 1; j++) {
			if (!labels[j]) continue;
			if (labels_up[j]) labels[j] = labels_up[j];
			else if (labels_up[j - 1]) {
				if (!labels_up[j + 1] || labels_up[j + 1] == labels_up[j - 1]) labels[j] = labels_up[j - 1];
				else {
					unsigned label = labelsolver.GetLabel(labels_up[j - 1]);
					unsigned label_other = labelsolver.GetLabel(labels_up[j + 1]);
					if (label != label_other) label = labelsolver.Merge(label, label_other);
					labels[j] = label;
				}
			}
			else if (labels_up[j + 1]) {
				if (!labels[j - 1] || labels[j - 1] == labels_up[j + 1]) labels[j] = labels_up[j + 1];
				else {
					unsigned label = labelsolver.GetLabel(labels_up[j + 1]);
					unsigned label_other = labelsolver.GetLabel(labels[j - 1]);
					if (label != label_other) label = labelsolver.Merge(label, label_other);
					labels[j] = label;
				}
			}
			else if (labels[j - 1]) labels[j] = labels[j - 1];
			else labels[j] = labelsolver.NewLabel();
		}
		if (1 < width && labels[width - 1]) {
			if (labels_up[width - 2]) labels[width - 1] = labels_up[width - 2];
			else if (labels_up[width - 1]) labels[width - 1] = labels_up[width - 1];
			else if (labels[width - 2]) labels[width - 1] = labels[width - 2];
			else labels[width - 1] = labelsolver.NewLabel();
		}
	}
	labelsolver.Flatten();
}
void Labeling_TS8(unsigned* dest, const unsigned int* source, int height, int width) {
	//initialize label state
	for (int i = 0; i < height; i++) {
		const unsigned int* datas = source + (size_t)i * width;
		unsigned int* labels = dest + (size_t)i * width;
		for (int j = 0; j < width; j++) {
			if (datas[j]) labels[j] = 1;
			else labels[j] = 0;
		}
	}

	//first pass
	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	CCL_TS8_FirstPass(dest, height, width, labelsolver);

	//second pass
	for (int i = 0; i < height; i++) {
		unsigned int* labels = dest + (size_t)i * width;
		for (int j = 0; j < width; j++) {
			if (labels[j]) labels[j] = labelsolver.GetLabel(labels[j]);
		}
	}

	//release memory 
	labelsolver.Dealloc();
}