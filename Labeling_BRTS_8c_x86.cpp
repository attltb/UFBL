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
		runs = new Run[height * ((size_t)width / 2 + 2) + 1];
	};
	~Runs() {
		delete[] runs;
	}
};
inline void CCL_BRTS8_X86_FindRuns(const unsigned int* bits_start, int height, int data_width, Run* runs, UFPC& labelsolver) {
	Run* runs_up = runs;

	//process runs in the first row
	const unsigned int* bits = bits_start;
	const unsigned int* bit_final = bits + data_width;
	unsigned int working_bits = *bits;
	unsigned int working_bits_r = ~working_bits;
	unsigned long basepos = 0, bitpos = 0;
	for (;; runs++) {
		//find starting position
		working_bits &= 0xFFFFFFFF << bitpos;
		while (!_BitScanForward(&bitpos, working_bits)) {
			bits++, basepos += 32;
			if (bits == bit_final) {
				runs->start_pos = (short)0xFFFF;
				runs->end_pos = (short)0xFFFF;
				runs++;
				goto out;
			}
			working_bits = *bits;
			working_bits_r = ~working_bits;
		}
		runs->start_pos = short(basepos + bitpos);

		//find ending position
		working_bits_r &= 0xFFFFFFFF << bitpos;
		while (!_BitScanForward(&bitpos, working_bits_r)) {
			bits++, basepos += 32;
			working_bits = *bits;
			working_bits_r = ~working_bits;
		}
		runs->end_pos = short(basepos + bitpos);
		runs->label = labelsolver.NewLabel();
	}
out:

	//process runs in the rests
	for (size_t row = 1; row < height; row++) {
		Run* runs_save = runs;
		const unsigned int* bits = bits_start + data_width * row;
		const unsigned int* bit_final = bits + data_width;
		unsigned int working_bits = *bits;
		unsigned int working_bits_r = ~working_bits;
		unsigned long basepos = 0, bitpos = 0;
		for (;; runs++) {
			//find starting position
			working_bits &= 0xFFFFFFFF << bitpos;
			while (!_BitScanForward(&bitpos, working_bits)) {
				bits++, basepos += 32;
				if (bits == bit_final) {
					runs->start_pos = (short)0xFFFF;
					runs->end_pos = (short)0xFFFF;
					runs++;
					goto out2;
				}
				working_bits = *bits;
				working_bits_r = ~working_bits;
			}
			unsigned short start_pos = short(basepos + bitpos);

			//find ending position
			working_bits_r &= 0xFFFFFFFF << bitpos;
			while (!_BitScanForward(&bitpos, working_bits_r)) {
				bits++, basepos += 32;
				working_bits = *bits;
				working_bits_r = ~working_bits;
			}
			unsigned short end_pos = short(basepos + bitpos);

			//Skip upper runs end before this run starts
			for (; runs_up->end_pos < start_pos; runs_up++);

			//No upper run meets this
			if (runs_up->start_pos > end_pos) {
				runs->start_pos = start_pos;
				runs->end_pos = end_pos;
				runs->label = labelsolver.NewLabel();
				continue;
			};

			unsigned label = labelsolver.GetLabel(runs_up->label);

			//Next upper run can not meet this
			if (end_pos <= runs_up->end_pos) {
				runs->start_pos = start_pos;
				runs->end_pos = end_pos;
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
			runs->start_pos = start_pos;
			runs->end_pos = end_pos;
			runs->label = label;
		}

	out2:
		runs_up = runs_save;
	}
	labelsolver.Flatten();
}

void Labeling_BRTS8_X86(unsigned* dest, const void* source, int height, int width, int alignment) {
	if (height == 0 || width == 0) return;
	int alignment_bits = 8 * alignment;
	int row_bytes = (width / alignment_bits + (width % alignment_bits != 0)) * alignment;

	const unsigned int* bits;
	int data_width;
	if (row_bytes % 4) {
		int byte_copy_size = width / 8 + (width % 8 != 0);

		data_width = (width / 32) + (width % 32 != 0);
		int byte_width = data_width * 4;
		int byte_padding = byte_width - byte_copy_size;
		unsigned int* bits_new = new unsigned int[(size_t)height * data_width];

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
	else data_width = row_bytes / 4, bits = (const unsigned int*)source;

	//find runs
	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	Runs Data_run(height, width);
	CCL_BRTS8_X86_FindRuns(bits, height, data_width, Data_run.runs, labelsolver);

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