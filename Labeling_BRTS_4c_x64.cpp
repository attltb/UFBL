#include "Label_Solver.h"
#include <intrin0.h>
#include <string.h>
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
		runs = new Run[height * (width / 2 + (size_t)2) + 1];
	};
	~Runs() {
		delete[] runs;
	}
};

inline Run* CCL_BRTS4_X64_FindRuns(const unsigned __int64* bits, const unsigned __int64* bit_final, Run* runs) {
	unsigned __int64 working_bits = *bits;
	unsigned long basepos = 0, bitpos = 0;
	for (;; runs++) {
		//find starting position
		while (!_BitScanForward64(&bitpos, working_bits)) {
			bits++, basepos += 64;
			if (bits == bit_final) {
				runs->start_pos = (short)0xFFFF;
				runs->end_pos = (short)0xFFFF;
				return runs + 1;
			}
			working_bits = *bits;
		}
		runs->start_pos = short(basepos + bitpos);

		//find ending position
		working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
		while (!_BitScanForward64(&bitpos, working_bits)) {
			bits++, basepos += 64;
			if (bits == bit_final) {
				runs->end_pos = short(basepos), runs++;
				runs->start_pos = (short)0xFFFF;
				runs->end_pos = (short)0xFFFF;
				return runs + 1;
			}
			working_bits = ~(*bits);
		}
		working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
		runs->end_pos = short(basepos + bitpos);
	}
}
inline void CCL_BRTS4_X64_GenerateLabelsOnRun(Run* runs, Run* runs_end, UFPC& labelsolver) {
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
			for (; runs_up->end_pos <= start_pos; runs_up++);

			//No upper run meets this
			unsigned short end_pos = runs->end_pos;
			if (runs_up->start_pos >= end_pos) {
				runs->label = labelsolver.NewLabel();
				continue;
			};

			unsigned label = labelsolver.GetLabel(runs_up->label);

			//Next upper run can not meet this
			if (end_pos - 1 <= runs_up->end_pos) {
				runs->label = label;
				continue;
			}

			//Find next upper runs meet this
			runs_up++;
			for (; runs_up->start_pos < end_pos; runs_up++) {
				unsigned label_other = labelsolver.GetLabel(runs_up->label);
				if (label != label_other) label = labelsolver.Merge(label, label_other);
				if (end_pos - 1 <= runs_up->end_pos) break;
			}
			runs->label = label;
		}
		runs_up = runs_save;
	}
	labelsolver.Flatten();
}

void Labeling_BRTS4_X64(unsigned* dest, const void* source, int height, int width, int alignment) {
	if (height == 0 || width == 0) return;
	int alignment_bits = 8 * alignment;
	int row_bytes = (width / alignment_bits + (width % alignment_bits != 0)) * alignment;

	const unsigned __int64* bits;
	int data_width;
	if (row_bytes % 8) {
		int byte_copy_size = width / 8 + (width % 8 != 0);

		data_width = (width / 64) + (width % 64 != 0);
		int byte_width = data_width * 8;
		int byte_padding = byte_width - byte_copy_size;
		unsigned __int64* bits_new = new unsigned __int64[(size_t)height * data_width];

		char* bits_dest = (char*)bits_new;
		const char* bits_source = (const char*)source;
		for (int i = 0; i < height; i++) {
			memcpy(bits_dest, bits_source, byte_copy_size);
			if (byte_padding) memset(bits_dest + byte_copy_size, 0, byte_padding);
			bits_dest += byte_width;
			bits_source += row_bytes;
		}
		bits = bits_new;
	}
	else data_width = row_bytes / 8, bits = (const unsigned __int64*)source;

	//find runs
	Runs Data_run(height, width);
	Run* run_next = Data_run.runs;
	const unsigned __int64* bits_m = bits;
	for (int i = 0; i < height; i++, bits_m += data_width) {
		run_next = CCL_BRTS4_X64_FindRuns(bits_m, bits_m + data_width, run_next);
	}

	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	CCL_BRTS4_X64_GenerateLabelsOnRun(Data_run.runs, run_next, labelsolver);

	//generate label data
	Run* runs = Data_run.runs;
	for (size_t i = 0; i < height; i++) {
		unsigned* labels = dest + width * i;
		for (size_t j = 0;; runs++) {
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
	if (bits != source) delete[] bits;
};