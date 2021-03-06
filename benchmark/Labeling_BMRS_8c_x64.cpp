#include "Label_Solver.h"
#include "Formats.h"
#include "Format_changer_X64.h"
#include <intrin.h>
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
inline unsigned long long CCL_BMRS_X64_is_connected(const unsigned long long* flag_bits, unsigned start, unsigned end) {
	if (start == end) return flag_bits[start >> 6] & ((unsigned long long)1 << (start & 0x0000003F));

	unsigned st_base = start >> 6;
	unsigned st_bits = start & 0x0000003F;
	unsigned ed_base = (end + 1) >> 6;
	unsigned ed_bits = (end + 1) & 0x0000003F;
	if (st_base == ed_base) {
		unsigned long long cutter = (0xFFFFFFFFFFFFFFFF << st_bits) ^ (0xFFFFFFFFFFFFFFFF << ed_bits);
		return flag_bits[st_base] & cutter;
	}

	for (unsigned i = st_base + 1; i < ed_base; i++) {
		if (flag_bits[i]) return true;
	}
	unsigned long long cutter_st = 0xFFFFFFFFFFFFFFFF << st_bits;
	unsigned long long cutter_ed = ~(0xFFFFFFFFFFFFFFFF << ed_bits);
	if (flag_bits[st_base] & cutter_st) return true;
	if (flag_bits[ed_base] & cutter_ed) return true;
	return false;
}
inline void CCL_BMRS_X64_FindRuns(const unsigned long long* bits_start, const unsigned long long* bits_flag, int height, int width, int qword_width, Run* runs, UFPC& labelsolver) {
	Run* runs_up = runs;

	//process runs in the first merged row
	const unsigned long long* bits = bits_start;
	const unsigned long long* bit_final = bits + (width / 64) + (width % 64 != 0);
	unsigned long long working_bits = *bits;
	unsigned long basepos = 0, bitpos = 0;
	for (;; runs++) {
		//find starting position
		while (!_BitScanForward64(&bitpos, working_bits)) {
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
		const unsigned long long* bits_f = bits_flag + qword_width * (row - 1);
		const unsigned long long* bits = bits_start + qword_width * row;
		const unsigned long long* bit_final = bits + (width / 64) + (width % 64 != 0);
		unsigned long long working_bits = *bits;
		unsigned long basepos = 0, bitpos = 0;

		for (;; runs++) {
			//find starting position
			while (!_BitScanForward64(&bitpos, working_bits)) {
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

void Labeling_BMRS_X64_on_MSB_First(unsigned* dest, const void* source, int height, int width, int data_width, int fmbits) {
	std::pair<const unsigned long long*, std::pair<int, int>> format_new = CCL_Format_Change_and_Bswap_X64(source, height, width, data_width, fmbits);

	const unsigned long long* bits = format_new.first;
	int qword_width_src = format_new.second.first;
	int qword_width_f = width / 64 + (width % 64 != 0);
	int qword_width_new = qword_width_f + 1;
	int base_r = format_new.second.second;

	//generate merged data
	int h_merge = height / 2 + height % 2;
	unsigned long long* bits_merged = new unsigned long long[(size_t)h_merge * qword_width_new];
	for (size_t i = 0; i < height / 2; i++) {
		unsigned long long* bits_dest = bits_merged + qword_width_new * i;
		const unsigned long long* bits_u = bits + qword_width_src * (2 * i);
		const unsigned long long* bits_d = bits_u + qword_width_src;
		for (int j = 0; j < qword_width_f; j++) bits_dest[j] = bits_u[j] | bits_d[j];
		bits_dest[qword_width_f] = 0;
	}
	if (height % 2) {
		unsigned long long* bits_dest = bits_merged + qword_width_new * ((size_t)height / 2);
		const unsigned long long* bits_u = bits + qword_width_src * ((size_t)height - 1);
		for (int j = 0; j < qword_width_f; j++) bits_dest[j] = bits_u[j];
		bits_dest[qword_width_f] = 0;
	}

	//generate flag bits
	int h_flag = h_merge - 1;
	unsigned long long* bit_flags = new unsigned long long[(size_t)h_flag * qword_width_new];
	for (size_t i = 0; i < (size_t)h_flag; i++) {
		const unsigned long long* bits_u = bits + qword_width_src * (2 * i + 1);
		const unsigned long long* bits_d = bits_u + qword_width_src;
		unsigned long long* bits_dest = bit_flags + qword_width_new * i;

		unsigned long long u0 = bits_u[0];
		unsigned long long d0 = bits_d[0];
		bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
		for (size_t j = 1; j < qword_width_f; j++) {
			unsigned long long u = bits_u[j];
			unsigned long long u_shl = u << 1;
			unsigned long long d = bits_d[j];
			unsigned long long d_shl = d << 1;
			if (bits_u[j - 1] & 0x8000000000000000) u_shl |= 1;
			if (bits_d[j - 1] & 0x8000000000000000) d_shl |= 1;
			bits_dest[j] = (u | u_shl) & (d | d_shl);
		}
		bits_dest[qword_width_f] = 0;
	}

	//find runs
	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	Runs Data_run(h_merge, width);
	CCL_BMRS_X64_FindRuns(bits_merged, bit_flags, h_merge, base_r, qword_width_new, Data_run.runs, labelsolver);

	//generate label data 
	Run* runs = Data_run.runs;
	int qword_width_in_byte = qword_width_src * sizeof(long long);
	for (int i = 0; i < height / 2; i++) {
		const char* data_u = (const char*)bits + (size_t)qword_width_in_byte * 2 * i;
		const char* data_d = data_u + qword_width_in_byte;
		unsigned* labels_u = dest + (size_t)width * 2 * i;
		unsigned* labels_d = labels_u + width;

		for (int j = width - 1;; runs++) {
			unsigned short start_pos = base_r - runs->start_pos;
			if (runs->start_pos == 0xFFFF) {
				for (int k = j; k >= 0; k--) labels_u[k] = 0;
				for (int k = j; k >= 0; k--) labels_d[k] = 0;
				runs++;
				break;
			}
			unsigned short end_pos = base_r - runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);

			for (; j >= start_pos; j--) labels_u[j] = 0, labels_d[j] = 0;
			for (int jr = base_r - 1 - j; j >= end_pos; j--, jr++) {
				labels_u[j] = (data_u[jr >> 3] & (1 << (jr & 0x07))) ? label : 0;
				labels_d[j] = (data_d[jr >> 3] & (1 << (jr & 0x07))) ? label : 0;
			}
		}
	}
	if (height % 2) {
		unsigned* labels = dest + width * (height - (size_t)1);
		for (int j = width - 1;; runs++) {
			unsigned short start_pos = base_r - runs->start_pos;
			if (runs->start_pos == 0xFFFF) {
				for (; j >= 0; j--) labels[j] = 0;
				break;
			}
			unsigned short end_pos = base_r - runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);
			for (; j >= start_pos; j--) labels[j] = 0;
			for (; j >= end_pos; j--) labels[j] = label;
		}
	}

	labelsolver.Dealloc();
	delete[] bit_flags;
	delete[] bits_merged;
	delete[] bits;
};
void Labeling_BMRS_X64(unsigned* dest, const void* source, int height, int width, int data_width, int fmbits) {
	if (fmbits & BTCPR_FM_MSB_FIRST) return Labeling_BMRS_X64_on_MSB_First(dest, source, height, width, data_width, fmbits);
	std::pair<const unsigned long long*, int> format_new = CCL_Format_Change_X64(source, height, width, data_width, fmbits);

	const unsigned long long* bits = format_new.first;
	int qword_width_src = format_new.second;
	int qword_width_f = width / 64;
	int qword_width_new = qword_width_f + 1;

	//generate merged data
	int h_merge = height / 2 + height % 2;
	unsigned long long* bits_merged = new unsigned long long[(size_t)h_merge * qword_width_new];
	for (size_t i = 0; i < height / 2; i++) {
		unsigned long long* bits_dest = bits_merged + qword_width_new * i;
		const unsigned long long* bits_u = bits + qword_width_src * (2 * i);
		const unsigned long long* bits_d = bits_u + qword_width_src;
		for (int j = 0; j < qword_width_f; j++) bits_dest[j] = bits_u[j] | bits_d[j];
		bits_dest[qword_width_f] = (width % 64) ? bits_u[qword_width_f] | bits_d[qword_width_f] : 0;
	}
	if (height % 2) {
		unsigned long long* bits_dest = bits_merged + qword_width_new * ((size_t)height / 2);
		const unsigned long long* bits_u = bits + qword_width_src * ((size_t)height - 1);
		for (int j = 0; j < qword_width_f; j++) bits_dest[j] = bits_u[j];
		bits_dest[qword_width_f] = (width % 64) ? bits_u[qword_width_f] : 0;
	}

	//generate flag bits
	int h_flag = h_merge - 1;
	unsigned long long* bit_flags = new unsigned long long[(size_t)h_flag * qword_width_new];
	for (size_t i = 0; i < (size_t)h_flag; i++) {
		const unsigned long long* bits_u = bits + qword_width_src * (2 * i + 1);
		const unsigned long long* bits_d = bits_u + qword_width_src;
		unsigned long long* bits_dest = bit_flags + qword_width_new * i;

		unsigned long long u0 = bits_u[0];
		unsigned long long d0 = bits_d[0];
		bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
		for (size_t j = 1; j < qword_width_f; j++) {
			unsigned long long u = bits_u[j];
			unsigned long long u_shl = u << 1;
			unsigned long long d = bits_d[j];
			unsigned long long d_shl = d << 1;
			if (bits_u[j - 1] & 0x8000000000000000) u_shl |= 1;
			if (bits_d[j - 1] & 0x8000000000000000) d_shl |= 1;
			bits_dest[j] = (u | u_shl) & (d | d_shl);
		}
		if (width % 64) {
			unsigned long long u = bits_u[qword_width_f];
			unsigned long long u_shl = u << 1;
			unsigned long long d = bits_d[qword_width_f];
			unsigned long long d_shl = d << 1;
			if (bits_u[qword_width_f - 1] & 0x8000000000000000) u_shl |= 1;
			if (bits_d[qword_width_f - 1] & 0x8000000000000000) d_shl |= 1;
			bits_dest[qword_width_f] = (u | u_shl) & (d | d_shl);
		}
		else bits_dest[qword_width_f] = 0;
	}

	//find runs
	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	Runs Data_run(h_merge, width);
	CCL_BMRS_X64_FindRuns(bits_merged, bit_flags, h_merge, width, qword_width_new, Data_run.runs, labelsolver);

	//generate label data 
	Run* runs = Data_run.runs;
	int qword_width_in_byte = qword_width_src * sizeof(long long);
	for (size_t i = 0; i < height / 2; i++) {
		const char* data_u = (const char*)bits + (size_t)qword_width_in_byte * 2 * i;
		const char* data_d = data_u + qword_width_in_byte;
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
	if (bits != source) delete[] bits;
};