#include "Label_Solver.h"
#include "BitScanner.h"
#include "Formats.h"
#include "Format_changer_X64.h"
void Labeling_BMRS_X64(unsigned* dest, const void* source, int height, int width, int data_width, int fmbits) {
	class BMRS {
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

		unsigned* const dest;
		const void* source;
		const int height;
		const int width;
		const int data_width;
		const int fmbits;

	public:
		BMRS(unsigned* _dest, const void* _source, int _height, int _width, int _data_width, int _fmbits)
			: dest(_dest), source(_source), height(_height), width(_width), data_width(_data_width), fmbits(_fmbits) {};
		void Perform()
		{
			if (fmbits & BTCPR_FM_MSB_FIRST) return Perform_on_MSB_First();
			std::pair<const uint64_t*, int> format_new = CCL_Format_Change_X64(source, height, width, data_width, fmbits);

			const uint64_t* bits = format_new.first;
			int qword_width_src = format_new.second;
			int qword_width_f = width / 64;
			int qword_width_new = qword_width_f + 1;

			//generate merged data
			int h_merge = height / 2 + height % 2;
			uint64_t* bits_merged = new uint64_t[h_merge * qword_width_new];
			for (int i = 0; i < height / 2; i++) {
				uint64_t* bits_dest = bits_merged + qword_width_new * i;
				const uint64_t* bits_u = bits + qword_width_src * (2 * i);
				const uint64_t* bits_d = bits_u + qword_width_src;
				for (int j = 0; j < qword_width_f; j++) bits_dest[j] = bits_u[j] | bits_d[j];
				bits_dest[qword_width_f] = (width % 64) ? bits_u[qword_width_f] | bits_d[qword_width_f] : 0;
			}
			if (height % 2) {
				uint64_t* bits_dest = bits_merged + qword_width_new * (height / 2);
				const uint64_t* bits_u = bits + qword_width_src * (height - 1);
				for (int j = 0; j < qword_width_f; j++) bits_dest[j] = bits_u[j];
				bits_dest[qword_width_f] = (width % 64) ? bits_u[qword_width_f] : 0;
			}

			//generate flag bits
			int h_flag = h_merge - 1;
			uint64_t* bit_flags = new uint64_t[h_flag * qword_width_new];
			for (int i = 0; i < h_flag; i++) {
				const uint64_t* bits_u = bits + qword_width_src * (2 * i + 1);
				const uint64_t* bits_d = bits_u + qword_width_src;
				uint64_t* bits_dest = bit_flags + qword_width_new * i;

				uint64_t u0 = bits_u[0];
				uint64_t d0 = bits_d[0];
				bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
				for (int j = 1; j < qword_width_f; j++) {
					uint64_t u = bits_u[j];
					uint64_t u_shl = u << 1;
					uint64_t d = bits_d[j];
					uint64_t d_shl = d << 1;
					if (bits_u[j - 1] & 0x8000000000000000) u_shl |= 1;
					if (bits_d[j - 1] & 0x8000000000000000) d_shl |= 1;
					bits_dest[j] = (u | u_shl) & (d | d_shl);
				}
				if (width % 64) {
					uint64_t u = bits_u[qword_width_f];
					uint64_t u_shl = u << 1;
					uint64_t d = bits_d[qword_width_f];
					uint64_t d_shl = d << 1;
					if (bits_u[qword_width_f - 1] & 0x8000000000000000) u_shl |= 1;
					if (bits_d[qword_width_f - 1] & 0x8000000000000000) d_shl |= 1;
					bits_dest[qword_width_f] = (u | u_shl) & (d | d_shl);
				}
				else bits_dest[qword_width_f] = 0;
			}

			//find runs
			UFPC labelsolver;
			labelsolver.Alloc(((height + 1) / 2) * ((width + 1) / 2) + 1);
			labelsolver.Setup();
			Runs Data_run(h_merge, width);
			FindRuns(bits_merged, bit_flags, h_merge, width, qword_width_new, Data_run.runs, labelsolver);

			//generate labels
			Generate(bits, qword_width_src, Data_run.runs, labelsolver);

			labelsolver.Dealloc();
			delete[] bit_flags;
			delete[] bits_merged;
			if (bits != source) delete[] bits;
		}
		void Perform_on_MSB_First() {
			std::pair<const uint64_t*, std::pair<int, int>> format_new = CCL_Format_Change_and_Bswap_X64(source, height, width, data_width, fmbits);

			const uint64_t* bits = format_new.first;
			int qword_width_src = format_new.second.first;
			int qword_width_f = width / 64 + (width % 64 != 0);
			int qword_width_new = qword_width_f + 1;
			int base_r = format_new.second.second;

			//generate merged data
			int h_merge = height / 2 + height % 2;
			uint64_t* bits_merged = new uint64_t[h_merge * qword_width_new];
			for (int i = 0; i < height / 2; i++) {
				uint64_t* bits_dest = bits_merged + qword_width_new * i;
				const uint64_t* bits_u = bits + qword_width_src * (2 * i);
				const uint64_t* bits_d = bits_u + qword_width_src;
				for (int j = 0; j < qword_width_f; j++) bits_dest[j] = bits_u[j] | bits_d[j];
				bits_dest[qword_width_f] = 0;
			}
			if (height % 2) {
				uint64_t* bits_dest = bits_merged + qword_width_new * (height / 2);
				const uint64_t* bits_u = bits + qword_width_src * (height - 1);
				for (int j = 0; j < qword_width_f; j++) bits_dest[j] = bits_u[j];
				bits_dest[qword_width_f] = 0;
			}

			//generate flag bits
			int h_flag = h_merge - 1;
			uint64_t* bit_flags = new uint64_t[h_flag * qword_width_new];
			for (int i = 0; i < h_flag; i++) {
				const uint64_t* bits_u = bits + qword_width_src * (2 * i + 1);
				const uint64_t* bits_d = bits_u + qword_width_src;
				uint64_t* bits_dest = bit_flags + qword_width_new * i;

				uint64_t u0 = bits_u[0];
				uint64_t d0 = bits_d[0];
				bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
				for (int j = 1; j < qword_width_f; j++) {
					uint64_t u = bits_u[j];
					uint64_t u_shl = u << 1;
					uint64_t d = bits_d[j];
					uint64_t d_shl = d << 1;
					if (bits_u[j - 1] & 0x8000000000000000) u_shl |= 1;
					if (bits_d[j - 1] & 0x8000000000000000) d_shl |= 1;
					bits_dest[j] = (u | u_shl) & (d | d_shl);
				}
				bits_dest[qword_width_f] = 0;
			}

			//find runs
			UFPC labelsolver;
			labelsolver.Alloc(((height + 1) / 2) * ((width + 1) / 2) + 1);
			labelsolver.Setup();
			Runs Data_run(h_merge, width);
			FindRuns(bits_merged, bit_flags, h_merge, base_r, qword_width_new, Data_run.runs, labelsolver);

			//generate labels
			Generate_on_MSB_First(base_r, bits, qword_width_src, Data_run.runs, labelsolver);

			labelsolver.Dealloc();
			delete[] bit_flags;
			delete[] bits_merged;
			delete[] bits;
		};

	private:
		uint64_t is_connected(const uint64_t* flag_bits, unsigned start, unsigned end) {
			if (start == end) return flag_bits[start >> 6] & ((uint64_t)1 << (start & 0x0000003F));

			unsigned st_base = start >> 6;
			unsigned st_bits = start & 0x0000003F;
			unsigned ed_base = (end + 1) >> 6;
			unsigned ed_bits = (end + 1) & 0x0000003F;
			if (st_base == ed_base) {
				uint64_t cutter = (0xFFFFFFFFFFFFFFFF << st_bits) ^ (0xFFFFFFFFFFFFFFFF << ed_bits);
				return flag_bits[st_base] & cutter;
			}

			for (unsigned i = st_base + 1; i < ed_base; i++) {
				if (flag_bits[i]) return true;
			}
			uint64_t cutter_st = 0xFFFFFFFFFFFFFFFF << st_bits;
			uint64_t cutter_ed = ~(0xFFFFFFFFFFFFFFFF << ed_bits);
			if (flag_bits[st_base] & cutter_st) return true;
			if (flag_bits[ed_base] & cutter_ed) return true;
			return false;
		}
		void FindRuns(const uint64_t* bits_start, const uint64_t* bits_flag, int h_new, int w_new, int qword_width, Run* runs, UFPC& labelsolver) {
			Run* runs_up = runs;

			//process runs in the first merged row
			const uint64_t* bits = bits_start;
			const uint64_t* bit_final = bits + (w_new / 64) + (w_new % 64 != 0);
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
					working_bits = ~(*bits);
				}
				working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
				runs->end_pos = short(basepos + bitpos);
				runs->label = labelsolver.NewLabel();
			}
		out:

			//process runs in the rests
			for (int row = 1; row < h_new; row++) {
				Run* runs_save = runs;
				const uint64_t* bits_f = bits_flag + qword_width * (row - 1);
				const uint64_t* bits = bits_start + qword_width * row;
				const uint64_t* bit_final = bits + (w_new / 64) + (w_new % 64 != 0);
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
						if (is_connected(bits_f, cross_st, end_pos)) runs->label = labelsolver.GetLabel(runs_up->label);
						else runs->label = labelsolver.NewLabel();
						runs->start_pos = start_pos;
						runs->end_pos = end_pos;
						continue;
					}

					unsigned label;
					if (is_connected(bits_f, cross_st, runs_up->end_pos)) label = labelsolver.GetLabel(runs_up->label);
					else label = 0;
					runs_up++;

					//Find next upper runs meet this
					for (; runs_up->start_pos <= end_pos; runs_up++) {
						if (end_pos <= runs_up->end_pos) {
							if (is_connected(bits_f, runs_up->start_pos, end_pos)) {
								unsigned label_other = labelsolver.GetLabel(runs_up->label);
								if (label != label_other) {
									label = (label) ? labelsolver.Merge(label, label_other) : label_other;
								}
							}
							break;
						}
						else {
							if (is_connected(bits_f, runs_up->start_pos, runs_up->end_pos)) {
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
		void Generate(const uint64_t* bits, int qword_width_src, const Run* runs, UFPC& labelsolver) {
			if (!(fmbits & BTCPR_FM_NO_ZEROINIT)) memset(dest, 0, height * width * sizeof(unsigned));

			int qword_width_in_byte = qword_width_src * sizeof(long long);
			if (!(fmbits & BTCPR_FM_NO_ZEROINIT) || (fmbits & BTCPR_FM_ZERO_BUF)) {
				for (int i = 0; i < height / 2; i++) {
					const uint64_t* const data_u = bits + qword_width_src * 2 * i;
					const uint64_t* const data_d = data_u + qword_width_src;
					unsigned* labels_u = dest + width * 2 * i;
					unsigned* labels_d = labels_u + width;


					for (;; runs++) {
						unsigned short start_pos = runs->start_pos;
						if (start_pos == 0xFFFF) {
							runs++;
							break;
						}
						unsigned short end_pos = runs->end_pos;
						unsigned label = labelsolver.GetLabel(runs->label);
						for (int j = start_pos; j < end_pos; j++) {
							if (data_u[j >> 6] & (1ull << (j & 0x3F))) labels_u[j] = label;
							if (data_d[j >> 6] & (1ull << (j & 0x3F))) labels_d[j] = label;
						}
					}
				}
			}
			else {
				for (int i = 0; i < height / 2; i++) {
					const uint64_t* const data_u = bits + qword_width_src * 2 * i;
					const uint64_t* const data_d = data_u + qword_width_src;
					unsigned* labels_u = dest + width * 2 * i;
					unsigned* labels_d = labels_u + width;

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
							labels_u[j] = (data_u[j >> 6] & (1ull << (j & 0x3F))) ? label : 0;
							labels_d[j] = (data_d[j >> 6] & (1ull << (j & 0x3F))) ? label : 0;
						}
					}
				}
			}

			if (height % 2) {
				unsigned* labels = dest + width * (height - 1);
				if (!(fmbits & BTCPR_FM_NO_ZEROINIT) || (fmbits & BTCPR_FM_ZERO_BUF)) {
					for (;; runs++) {
						unsigned short start_pos = runs->start_pos;
						if (start_pos == 0xFFFF) {
							break;
						}
						unsigned short end_pos = runs->end_pos;
						unsigned label = labelsolver.GetLabel(runs->label);
						for (int j = start_pos; j < end_pos; j++) labels[j] = label;
					}
				}
				else {
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
			}
		}
		void Generate_on_MSB_First(int base_r, const uint64_t* bits, int qword_width_src, const Run* runs, UFPC& labelsolver) {
			if (!(fmbits & BTCPR_FM_NO_ZEROINIT)) memset(dest, 0, height * width * sizeof(unsigned));

			int qword_width_in_byte = qword_width_src * sizeof(long long);
			if (!(fmbits & BTCPR_FM_NO_ZEROINIT) || (fmbits & BTCPR_FM_ZERO_BUF)) {
				for (int i = 0; i < height / 2; i++) {
					const uint64_t* const data_u = bits + qword_width_src * 2 * i;
					const uint64_t* const data_d = data_u + qword_width_src;
					unsigned* labels_u = dest + width * 2 * i;
					unsigned* labels_d = labels_u + width;

					for (int j = width - 1;; runs++) {
						unsigned short start_pos = base_r - runs->start_pos;
						if (runs->start_pos == 0xFFFF) {
							runs++;
							break;
						}
						unsigned short end_pos = base_r - runs->end_pos;
						unsigned label = labelsolver.GetLabel(runs->label);
						j = start_pos - 1;
						for (int jr = base_r - 1 - j; j >= end_pos; j--, jr++) {
							if (data_u[jr >> 6] & (1ull << (jr & 0x3F))) labels_u[j] = label;
							if (data_d[jr >> 6] & (1ull << (jr & 0x3F))) labels_d[j] = label;
						}
					}
				}
			}
			else {
				for (int i = 0; i < height / 2; i++) {
					const uint64_t* const data_u = bits + qword_width_src * 2 * i;
					const uint64_t* const data_d = data_u + qword_width_src;
					unsigned* labels_u = dest + width * 2 * i;
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
							labels_u[j] = (data_u[jr >> 6] & (1ull << (jr & 0x3F))) ? label : 0;
							labels_d[j] = (data_d[jr >> 6] & (1ull << (jr & 0x3F))) ? label : 0;
						}
					}
				}
			}

			if (height % 2) {
				unsigned* labels = dest + width * (height - 1);
				if (!(fmbits & BTCPR_FM_NO_ZEROINIT) || (fmbits & BTCPR_FM_ZERO_BUF)) {
					for (int j = width - 1;; runs++) {
						unsigned short start_pos = base_r - runs->start_pos;
						if (runs->start_pos == 0xFFFF) {
							break;
						}
						unsigned short end_pos = base_r - runs->end_pos;
						unsigned label = labelsolver.GetLabel(runs->label);
						for (; j >= end_pos; j--) labels[j] = label;
					}
				}
				else {
					for (int j = width - 1;; runs++) {
						unsigned short start_pos = base_r - runs->start_pos;
						if (runs->start_pos == 0xFFFF) {
							for (; j >= 0; j--) labels[j] = 0;
							break;
						}
						unsigned short end_pos = base_r - runs->end_pos;
						unsigned label = labelsolver.GetLabel(runs->label);
						for (j = start_pos - 1; j >= end_pos; j--) labels[j] = label;
					}
				}
			}
		}
	};
	BMRS(dest, source, height, width, data_width, fmbits).Perform();
};
void Labeling_BMRS_on_byte_X64(unsigned* dest, const uint8_t* source, int height, int width, int data_width, int fmbits) {
	std::pair<const uint64_t*, int> format_compressed = CCL_Bit_Compressing_X64(source, height, width, data_width);

	Labeling_BMRS_X64(dest, format_compressed.first, height, width, format_compressed.second, BTCPR_FM_ALIGN_8 | BTCPR_FM_PADDING_ZERO | fmbits);
	delete[] format_compressed.first;
}