#include "Label_Solver.h"
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
		runs = new Run[(size_t)height * ((size_t)width / 2 + 2) + 1];
	};
	~Runs() {
		delete[] runs;
	}
};
inline unsigned __int64 CCL_BMRS_X64_is_connected(const unsigned __int64* flag_bits, unsigned start, unsigned end) {
	if (start == end) return flag_bits[start >> 6] & ((unsigned __int64)1 << (start & 0x0000003F));

	unsigned st_base = start >> 6;
	unsigned st_bits = start & 0x0000003F;
	unsigned ed_base = (end + 1) >> 6;
	unsigned ed_bits = (end + 1) & 0x0000003F;
	if (st_base == ed_base) {
		unsigned __int64 cutter = (0xFFFFFFFFFFFFFFFF << st_bits) ^ (0xFFFFFFFFFFFFFFFF << ed_bits);
		return flag_bits[st_base] & cutter;
	}

	for (unsigned i = st_base + 1; i < ed_base; i++) {
		if (flag_bits[i]) return true;
	}
	unsigned __int64 cutter_st = 0xFFFFFFFFFFFFFFFF << st_bits;
	unsigned __int64 cutter_ed = ~(0xFFFFFFFFFFFFFFFF << ed_bits);
	if (flag_bits[st_base] & cutter_st) return true;
	if (flag_bits[ed_base] & cutter_ed) return true;
	return false;
}
inline void CCL_BMRS_X64_FindRuns(const unsigned __int64* bits_start, const unsigned __int64* bits_flag, int height, int data_width, Run* runs, UFPC& labelsolver) {
	Run* runs_up = runs;

	//process runs in the first merged row
	const unsigned __int64* bits = bits_start;
	const unsigned __int64* bit_final = bits + data_width;
	unsigned __int64 working_bits = *bits;
	unsigned long basepos = 0, bitpos = 0;
	for (;; runs++) {
		//find starting position
		while (!_BitScanForward64(&bitpos, working_bits)) {
			bits++, basepos += 64;
			if (bits == bit_final) {
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
		while (!_BitScanForward64(&bitpos, working_bits)) {
			bits++, basepos += 64;
			working_bits = ~(*bits);
		}
		working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
		runs->end_pos = short(basepos + bitpos);
		runs->label = labelsolver.NewLabel();
	}
out:

	//process runs in the rests
	for (size_t row = 1; row < height; row++) {
		Run* runs_save = runs;
		const unsigned __int64* bits_f = bits_flag + data_width * (row - 1);
		const unsigned __int64* bits = bits_start + data_width * row;
		const unsigned __int64* bit_final = bits + data_width;
		unsigned __int64 working_bits = *bits;
		unsigned long basepos = 0, bitpos = 0;

		for (;; runs++) {
			//find starting position
			while (!_BitScanForward64(&bitpos, working_bits)) {
				bits++, basepos += 64;
				if (bits == bit_final) {
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
			while (!_BitScanForward64(&bitpos, working_bits)) {
				bits++, basepos += 64;
				working_bits = ~(*bits);
			}
			working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
			unsigned short end_pos = short(basepos + bitpos);

			//Skip upper runs end before this slice starts
			for (; runs_up->end_pos < start_pos; runs_up++);

			//No upper run meets this
			if (runs_up->start_pos > end_pos) {
				runs->start_pos = start_pos;
				runs->end_pos = end_pos;
				runs->label = labelsolver.NewLabel();
				continue;
			};

			//Next upper run can not meet this
			unsigned short cross_st = (start_pos >= runs_up->start_pos) ? start_pos : runs_up->start_pos;
			if (end_pos <= runs_up->end_pos) {
				if (CCL_BMRS_X64_is_connected(bits_f, cross_st, end_pos)) runs->label = labelsolver.GetLabel(runs_up->label);
				else runs->label = labelsolver.NewLabel();
				runs->start_pos = start_pos;
				runs->end_pos = end_pos;
				continue;
			}

			unsigned label;
			if (CCL_BMRS_X64_is_connected(bits_f, cross_st, runs_up->end_pos)) label = labelsolver.GetLabel(runs_up->label);
			else label = 0;
			runs_up++;

			//Find next upper runs meet this
			for (; runs_up->start_pos <= end_pos; runs_up++) {
				if (end_pos <= runs_up->end_pos) {
					if (CCL_BMRS_X64_is_connected(bits_f, runs_up->start_pos, end_pos)) {
						unsigned label_other = labelsolver.GetLabel(runs_up->label);
						if (label != label_other) {
							label = (label) ? labelsolver.Merge(label, label_other) : label_other;
						}
					}
					break;
				}
				else {
					if (CCL_BMRS_X64_is_connected(bits_f, runs_up->start_pos, runs_up->end_pos)) {
						unsigned label_other = labelsolver.GetLabel(runs_up->label);
						if (label != label_other) {
							label = (label) ? labelsolver.Merge(label, label_other) : label_other;
						}
					}
				}
			}

			if (label) runs->label = label;
			else runs->label = labelsolver.NewLabel();
			runs->start_pos = start_pos;
			runs->end_pos = end_pos;
		}
	out2:
		runs_up = runs_save;
	}
	labelsolver.Flatten();
}

void Labeling_BMRS_X64(unsigned* dest, const void* source, int height, int width, int alignment) {
	if (height == 0 || width == 0) return;
	int alignment_bits = 8 * alignment;
	int row_bytes = (width / alignment_bits + (width % alignment_bits != 0)) * alignment;

	int data_width_m1 = (width / 64);
	int data_width = data_width_m1 + 1;
	int edge_start = data_width_m1 * 8;
	int edge_end = ((width / 8) + (width % 8 != 0));

	//generate merged data
	int h_merge = height / 2 + height % 2;
	unsigned __int64* bits_merged = new unsigned __int64[(size_t)h_merge * data_width];
	for (size_t i = 0; i < height / 2; i++) {
		unsigned __int64* dest_m = bits_merged + data_width * i;
		const char* source1 = (const char*)source + row_bytes * (2 * i);
		const char* source2 = source1 + row_bytes;

		for (size_t j = 0; j < data_width_m1; j++) {
			dest_m[j] = ((const unsigned __int64*)source1)[j] | ((const unsigned __int64*)source2)[j];
		}
		dest_m[data_width_m1] = 0;
		for (int j = edge_start; j < edge_end; j++) ((char*)dest_m)[j] = source1[j] | source2[j];
	}
	if (height % 2) {
		unsigned __int64* dest_m = bits_merged + data_width * (height / (size_t)2);
		const char* source1 = (const char*)source + row_bytes * (height - (size_t)1);

		for (size_t j = 0; j < data_width_m1; j++) dest_m[j] = ((const unsigned __int64*)source1)[j];
		dest_m[data_width_m1] = 0;
		for (size_t j = edge_start; j < edge_end; j++) ((char*)dest_m)[j] = source1[j];
	}

	//generate flag bits
	unsigned __int64* bit_flags = new unsigned __int64[((size_t)h_merge - 1) * data_width];
	for (size_t i = 0; i < (size_t)h_merge - 1; i++) {
		const char* source_u = (const char*)source + row_bytes * (2 * i + 1);
		const char* source_d = source_u + row_bytes;
		const unsigned __int64* bits_u = (const unsigned __int64*)source_u;
		const unsigned __int64* bits_d = (const unsigned __int64*)source_d;
		unsigned __int64* bits_dest = bit_flags + data_width * i;

		unsigned __int64 u0 = bits_u[0];
		unsigned __int64 d0 = bits_d[0];
		bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
		for (size_t j = 1; j < data_width_m1; j++) {
			unsigned __int64 u = bits_u[j];
			unsigned __int64 u_shl = u << 1;
			unsigned __int64 d = bits_d[j];
			unsigned __int64 d_shl = d << 1;
			if (bits_u[j - 1] & 0x8000000000000000) u_shl |= 1;
			if (bits_d[j - 1] & 0x8000000000000000) d_shl |= 1;
			bits_dest[j] = (u | u_shl) & (d | d_shl);
		}
		bits_dest[data_width_m1] = 0;
		for (size_t j = edge_start; j < edge_end; j++) {
			char u = source_u[j];
			char u_shl = u << 1;
			char d = source_d[j];
			char d_shl = d << 1;
			if (source_u[j - 1] & 0x80) u_shl |= 1;
			if (source_d[j - 1] & 0x80) d_shl |= 1;
			((char*)bits_dest)[j] = (u | u_shl) & (d | d_shl);
		}
	}

	//find runs
	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	Runs Data_run(h_merge, width);
	CCL_BMRS_X64_FindRuns(bits_merged, bit_flags, h_merge, data_width, Data_run.runs, labelsolver);

	//generate label data 
	Run* runs = Data_run.runs;
	for (size_t i = 0; i < height / 2; i++) {
		const char* data_u = (const char*)source + (size_t)row_bytes * 2 * i;
		const char* data_d = data_u + row_bytes;
		unsigned* labels_u = dest + (size_t)width * 2 * i;
		unsigned* labels_d = labels_u + width;

		for (size_t j = 0;; runs++) {
			unsigned short start_pos = runs->start_pos;
			if (start_pos == 0xFFFF) {
				for (size_t k = j; k < width; k++) labels_u[k] = 0;
				for (size_t k = j; k < width; k++) labels_d[k] = 0;
				runs++;
				break;
			}
			unsigned short end_pos = runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);
			
			for (; j < start_pos; j++) labels_u[j] = 0, labels_d[j] = 0;
			for (; j < end_pos; j++) {
				labels_u[j] = (data_u[j >> 3] & (1 << (j & 0x07))) ? label : 0;
				labels_d[j] = (data_d[j >> 3] & (1 << (j & 0x07))) ? label : 0;
			}
		}
	}
	if (height % 2) {
		unsigned* labels = dest + width * (height - (size_t)1);
		for (size_t j = 0;; runs++) {
			unsigned short start_pos = runs->start_pos;
			if (start_pos == 0xFFFF) {
				for (size_t k = j; k < width; k++) labels[k] = 0;
				break;
			}
			unsigned short end_pos = runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);
			for (; j < start_pos; j++) labels[j] = 0;
			for (j = start_pos; j < end_pos; j++) labels[j] = label;
		}
	}

	labelsolver.Dealloc();
	delete[] bit_flags;
	delete[] bits_merged;
};