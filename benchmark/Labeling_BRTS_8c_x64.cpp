#include "Label_Solver.h"
#include "BitScanner.h"
#include "Formats.h"
#include "Format_changer_X64.h"
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
inline void CCL_BRTS8_X64_FindRuns(const uint64_t* bits_start, int height, int width, int qword_width, Run* runs, UFPC& labelsolver) {
	Run* runs_up = runs;

	//process runs in the first row
	const uint64_t* bits = bits_start;
	const uint64_t* bit_final = bits + width / 64 + (width % 64 != 0);
	uint64_t working_bits = *bits;
	unsigned long basepos = 0, bitpos = 0;
	for (;; runs++) {
		//find starting position
		while (!myBitScanForward64(&bitpos, working_bits)) {
			bits++, basepos += 64;
			if (bits >= bit_final) {
				runs->start_pos = (short)0xFFFF;
				runs->end_pos = (short)0xFFFF;
				runs++;
				goto out;
			}
			working_bits = *bits;
		}
		runs->start_pos = short(basepos + bitpos);

		//find ending position
		working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
		while (!myBitScanForward64(&bitpos, working_bits)) {
			bits++, basepos += 64;
			if (bits == bit_final) {
				bitpos = 0;
				working_bits = 0xFFFFFFFFFFFFFFFF;
				break;
			}
			working_bits = ~(*bits);
		}
		working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
		runs->end_pos = short(basepos + bitpos);
		runs->label = labelsolver.NewLabel();
	}
out:

	//process runs in the rests
	for (int row = 1; row < height; row++) {
		Run* runs_save = runs;
		const uint64_t* bits = bits_start + qword_width * row;
		const uint64_t* bit_final = bits + width / 64 + (width % 64 != 0);
		uint64_t working_bits = *bits;
		unsigned long basepos = 0, bitpos = 0;
		for (;; runs++) {
			//find starting position
			while (!myBitScanForward64(&bitpos, working_bits)) {
				bits++, basepos += 64;
				if (bits >= bit_final) {
					runs->start_pos = (short)0xFFFF;
					runs->end_pos = (short)0xFFFF;
					runs++;
					goto out2;
				}
				working_bits = *bits;
			}
			unsigned short start_pos = short(basepos + bitpos);

			//find ending position
			working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
			while (!myBitScanForward64(&bitpos, working_bits)) {
				bits++, basepos += 64;
				if (bits == bit_final) {
					bitpos = 0;
					working_bits = 0xFFFFFFFFFFFFFFFF;
					break;
				}
				working_bits = ~(*bits);
			}
			working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
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

void Labeling_BRTS8_X64_on_MSB_First(unsigned* dest, const void* source, int height, int width, int data_width, int fmbits) {
	std::pair<const uint64_t*, std::pair<int, int>> format_new = CCL_Format_Change_and_Bswap_X64(source, height, width, data_width, fmbits);

	const uint64_t* bits = format_new.first;
	int qword_width = format_new.second.first;
	int base_r = format_new.second.second;

	//find runs
	UFPC labelsolver;
	labelsolver.Alloc(((height + 1) / 2) * ((width + 1) / 2) + 1);
	labelsolver.Setup();
	Runs Data_run(height, width);
	CCL_BRTS8_X64_FindRuns(bits, height, base_r, qword_width, Data_run.runs, labelsolver);

	//generate label data
	Run* runs = Data_run.runs;
	for (int i = 0; i < height; i++) {
		unsigned* labels = dest + width * i;
		for (int j = width - 1;; runs++) {
			unsigned short start_pos = base_r - runs->start_pos;
			if (runs->start_pos == 0xFFFF) {
				for (; j >= 0; j--) labels[j] = 0;
				runs++;
				break;
			}
			unsigned short end_pos = base_r - runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);
			for (; j >= start_pos; j--) labels[j] = 0;
			for (; j >= end_pos; j--) labels[j] = label;
		}
	}

	labelsolver.Dealloc();
	delete[] bits;
}
void Labeling_BRTS8_X64(unsigned* dest, const void* source, int height, int width, int data_width, int fmbits) {
	if (fmbits & BTCPR_FM_MSB_FIRST) return Labeling_BRTS8_X64_on_MSB_First(dest, source, height, width, data_width, fmbits);
	std::pair<const uint64_t*, int> format_new = CCL_Format_Change_X64(source, height, width, data_width, fmbits);

	const uint64_t* bits = format_new.first;
	int qword_width = format_new.second;

	//find runs
	UFPC labelsolver;
	labelsolver.Alloc(((height + 1) / 2) * ((width + 1) / 2) + 1);
	labelsolver.Setup();
	Runs Data_run(height, width);
	CCL_BRTS8_X64_FindRuns(bits, height, width, qword_width, Data_run.runs, labelsolver);

	//generate label data
	Run* runs = Data_run.runs;
	for (int i = 0; i < height; i++) {
		unsigned* labels = dest + width * i;
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
	if (bits != source) delete[] bits;
};