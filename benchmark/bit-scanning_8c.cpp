#include "Common.h"
#include <intrin0.h>
struct Run {
	unsigned short start_pos;
	unsigned short end_pos;
	unsigned label;
};
struct Runs {
	Run* runs;
	unsigned height;
	unsigned width;
	Runs(unsigned _height, unsigned _width) : height(_height), width(_width) {
		runs = new Run[height * (width / 2 + 2) + 1];
	};
	~Runs() {
		delete[] runs;
	}
};

inline Run* CCL_BRTS8_FindRuns(const unsigned int* bits, const unsigned int* bit_final, Run* runs) {
	unsigned int working_bits = *bits;
	unsigned long basepos = 0, bitpos = 0;
	for (;; runs++) {
		//find starting position
		while (!_BitScanForward(&bitpos, working_bits)) {
			bits++, basepos += 32;
			if (bits == bit_final) {
				runs->start_pos = (short)0xFFFF;
				runs->end_pos = (short)0xFFFF;
				return runs + 1;
			}
			working_bits = *bits;
		}
		runs->start_pos = short(basepos + bitpos);

		//find ending position
		working_bits = (~working_bits) & (0xFFFFFFFF << bitpos);
		while (!_BitScanForward(&bitpos, working_bits)) {
			bits++, basepos += 32;
			working_bits = ~(*bits);
		}
		working_bits = (~working_bits) & (0xFFFFFFFF << bitpos);
		runs->end_pos = short(basepos + bitpos);
	}
}
inline void CCL_BRTS8_GenerateLabelsOnRun(Run* runs, Run* runs_end, UFPC& labelsolver) {
	Run* runs_up = runs;
	
	//generate labels for the first row
	for (; runs->start_pos != 0xFFFF; runs++) runs->label = labelsolver.NewLabel();
	runs++;

	//generate labels for the rests
	for (; runs != runs_end; runs++) {
		Run* runs_save = runs;
		for (;; runs++) {
			unsigned short start_pos = runs->start_pos;
			if (start_pos == 0xFFFF) break;

			//Skip upper runs end before this run starts 
			for (; runs_up->end_pos < start_pos; runs_up++);

			//No upper run meets this
			unsigned short end_pos = runs->end_pos;
			if (runs_up->start_pos > end_pos) {
				runs->label = labelsolver.NewLabel();
				continue;
			};

			unsigned label = labelsolver.GetLabel(runs_up->label);

			//Next upper run can not meet this
			if (end_pos <= runs_up->end_pos) {
				runs->label = label;
				continue;
			}

			//Find next upper runs meet this
			runs_up++;
			for (; runs_up->start_pos <= end_pos; runs_up++) {
				unsigned label_other = labelsolver.GetLabel(runs_up->label);
				if (label != label_other) label = labelsolver.Merge(label, label_other);
				if (end_pos <= runs_up->end_pos) break;
			}
			runs->label = label;
		}
		runs_up = runs_save;
	}
	labelsolver.Flatten();
}

void bit_scanning_algorithm_8c(const Data_Compressed& data, Data& data_labels) {
	int height = data.height;
	int width = data.width;

	//find runs
	Runs Data_run(height, width);
	Run* run_next = Data_run.runs;
	for (int i = 0; i < height; i++) {
		run_next = CCL_BRTS8_FindRuns(data[i], data[i + 1], run_next);
	}

	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	CCL_BRTS8_GenerateLabelsOnRun(Data_run.runs, run_next, labelsolver);

	//generate label data
	Run* runs = Data_run.runs;
	for (int i = 0; i < height; i++) {
		unsigned int* labels = data_labels.data[i];
		for (int j = 0;; runs++) {
			unsigned short start_pos = runs->start_pos;
			if (start_pos == 0xFFFF) {
				for (; j < width; j++) labels[j] = 0;
				runs++;
				break;
			}
			unsigned short end_pos = runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);
			for (; j < start_pos; j++) labels[j] = 0;
			for (; j < end_pos; j++) labels[j] = label;
		}
	}

	labelsolver.Dealloc();
};