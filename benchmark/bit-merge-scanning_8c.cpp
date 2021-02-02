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

inline Run* CCL_BMRS_FindRuns(const unsigned int* bits, const unsigned int* bit_final, Run* runs) {
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
inline unsigned int CCL_BMRS_is_connected(const unsigned int* flag_bits, unsigned start, unsigned end) {
	if (start == end) return flag_bits[start >> 5] & (1 << (start & 0x0000001F));

	unsigned st_base = start >> 5;
	unsigned st_bits = start & 0x0000001F;
	unsigned ed_base = (end + 1) >> 5;
	unsigned ed_bits = (end + 1) & 0x0000001F;
	if (st_base == ed_base) {
		unsigned int cutter = (0xFFFFFFFF << st_bits) ^ (0xFFFFFFFF << ed_bits);
		return flag_bits[st_base] & cutter;
	}

	for (unsigned i = st_base + 1; i < ed_base; i++) {
		if (flag_bits[i]) return true;
	}
	unsigned int cutter_st = 0xFFFFFFFF << st_bits;
	unsigned int cutter_ed = ~(0xFFFFFFFF << ed_bits);
	if (flag_bits[st_base] & cutter_st) return true;
	if (flag_bits[ed_base] & cutter_ed) return true;
	return false;
}
inline void CCL_BMRS_GenerateLabelsOnRun(const Data_Compressed& dt_flags, Run* runs, Run* runs_end, UFPC& labelsolver) {
	Run* runs_up = runs;
	
	//generate labels for the first row
	for (; runs->start_pos != 0xFFFF; runs++) runs->label = labelsolver.NewLabel();
	runs++;

	//generate labels for the rests
	int data_width = dt_flags.data_width;
	const unsigned int* ibits = dt_flags.bits;
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

			//Next upper run can not meet this
			unsigned short cross_st = (start_pos >= runs_up->start_pos) ? start_pos : runs_up->start_pos;
			if (end_pos <= runs_up->end_pos) {
				if (CCL_BMRS_is_connected(flags, cross_st, end_pos)) runs->label = labelsolver.GetLabel(runs_up->label);
				else runs->label = labelsolver.NewLabel();
				continue;
			}

			unsigned label;
			if (CCL_BMRS_is_connected(flags, cross_st, runs_up->end_pos)) label = labelsolver.GetLabel(runs_up->label);
			else label = 0;
			runs_up++;

			//Find next upper runs meet this
			for (; runs_up->start_pos <= end_pos; runs_up++) {
				if (end_pos <= runs_up->end_pos) {
					if (CCL_BMRS_is_connected(flags, runs_up->start_pos, end_pos)) {
						unsigned label_other = labelsolver.GetLabel(runs_up->label);
						if (label != label_other) {
							label = (label) ? labelsolver.Merge(label, label_other) : label_other;
						}
					}
					break;
				}
				else {
					if (CCL_BMRS_is_connected(flags, runs_up->start_pos, runs_up->end_pos)) {
						unsigned label_other = labelsolver.GetLabel(runs_up->label);
						if (label != label_other) {
							label = (label) ? labelsolver.Merge(label, label_other) : label_other;
						}
					}
				}
			}

			if (label) runs->label = label;
			else runs->label = labelsolver.NewLabel();
		}
		runs_up = runs_save;
		ibits += data_width;
	}
	labelsolver.Flatten();
}

void bit_merge_scanning_algorithm(const Data_Compressed& data, Data& data_labels) {
	int height = data.height;
	int width = data.width;

	//generate merged data
	int h_merge = height / 2 + height % 2;
	int data_width = data.data_width;
	Data_Compressed dt_merged(h_merge, width);
	for (int i = 0; i < height / 2; i++) {
		const unsigned int* data_source1 = data[2 * i];
		const unsigned int* data_source2 = data[2 * i + 1];
		unsigned int* data_merged = dt_merged[i];
		for (int j = 0; j < data_width; j++) data_merged[j] = data_source1[j] | data_source2[j];
	}
	if (height % 2) {
		const unsigned int* data_source = data[height - 1];
		unsigned int* data_merged = dt_merged[height / 2];
		for (int j = 0; j < data_width; j++) data_merged[j] = data_source[j];
	}

	//generate flag bits
	Data_Compressed dt_flags(h_merge - 1, width);
	for (int i = 0; i < dt_flags.height; i++) {
		const unsigned int* bits_u = data[2 * i + 1];
		const unsigned int* bits_d = data[2 * i + 2];
		unsigned int* bits_dest = dt_flags[i];

		unsigned int u0 = bits_u[0];
		unsigned int d0 = bits_d[0];
		bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
		for (int j = 1; j < data_width; j++) {
			unsigned int u = bits_u[j];
			unsigned int u_shl = u << 1;
			unsigned int d = bits_d[j];
			unsigned int d_shl = d << 1;
			if (bits_u[j - 1] & 0x80000000) u_shl |= 1;
			if (bits_d[j - 1] & 0x80000000) d_shl |= 1;
			bits_dest[j] = (u | u_shl) & (d | d_shl);
		}
	}

	//find runs
	Runs Data_run(h_merge, width);
	Run* run_next = Data_run.runs;
	for (int i = 0; i < h_merge; i++) {
		run_next = CCL_BMRS_FindRuns(dt_merged[i], dt_merged[i + 1], run_next);
	}

	//create label table
	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	CCL_BMRS_GenerateLabelsOnRun(dt_flags, Data_run.runs, run_next, labelsolver);

	//generate label data
	Run* runs = Data_run.runs;
	for (int i = 0; i < height / 2; i++) {
		const unsigned int* data_u = data[2 * i];
		const unsigned int* data_d = data[2 * i + 1];

		unsigned int* labels_u = data_labels.data[2 * i];
		unsigned int* labels_d = data_labels.data[2 * i + 1];
		for (int j = 0;; runs++) {
			unsigned short start_pos = runs->start_pos;
			if (start_pos == 0xFFFF) {
				for (int k = j; k < width; k++) labels_u[k] = 0;
				for (int k = j; k < width; k++) labels_d[k] = 0;
				runs++;
				break;
			}
			unsigned short end_pos = runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);

			for (; j < start_pos; j++) labels_u[j] = 0, labels_d[j] = 0;
			for (; j < end_pos; j++) {
				labels_u[j] = (data_u[j >> 5] & (1 << (j & 0x1F))) ? label : 0;
				labels_d[j] = (data_d[j >> 5] & (1 << (j & 0x1F))) ? label : 0;
			}
		}
	}
	if (height % 2) {
		unsigned int* labels = data_labels.data[height - 1];
		for (int j = 0;; runs++) {
			unsigned short start_pos = runs->start_pos;
			if (start_pos == 0xFFFF) {
				for (int k = j; k < width; k++) labels[k] = 0;
				break;
			}
			unsigned short end_pos = runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);
			for (; j < start_pos; j++) labels[j] = 0;
			for (j = start_pos; j < end_pos; j++) labels[j] = label;
		}
	}

	labelsolver.Dealloc();
};