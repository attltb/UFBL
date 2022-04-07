#include "Label_Solver.h"
#include "BitScanner.h"
#include "Formats.h"
#include "Format_changer_X86.h"
void Labeling_BRTS8_X86(unsigned* dest, const void* source, int height, int width, int data_width, int fmbits) {
	class BRTS8 {
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
		BRTS8(unsigned* _dest, const void* _source, int _height, int _width, int _data_width, int _fmbits)
			: dest(_dest), source(_source), height(_height), width(_width), data_width(_data_width), fmbits(_fmbits) {};
		void Perform()
		{
			if (fmbits & BTCPR_FM_MSB_FIRST) return Perform_on_MSB_First();
			std::pair<const uint32_t*, int> format_new = CCL_Format_Change_X86(source, height, width, data_width, fmbits);

			const uint32_t* bits = format_new.first;
			int dword_width = format_new.second;

			//find runs
			UFPC labelsolver;
			labelsolver.Alloc(((height + 1) / 2) * ((width + 1) / 2) + 1);
			labelsolver.Setup();
			Runs Data_run(height, width);
			FindRuns(bits, width, dword_width, Data_run.runs, labelsolver);

			//generate labels
			Generate(Data_run.runs, labelsolver);

			labelsolver.Dealloc();
			if (bits != source) delete[] bits;
		}
		void Perform_on_MSB_First()
		{
			std::pair<const uint32_t*, std::pair<int, int>> format_new = CCL_Format_Change_and_Bswap_X86(source, height, width, data_width, fmbits);

			const uint32_t* bits = format_new.first;
			int width_new = format_new.second.second;
			int dword_width = format_new.second.first;

			//find runs
			UFPC labelsolver;
			labelsolver.Alloc(((height + 1) / 2) * ((width + 1) / 2) + 1);
			labelsolver.Setup();
			Runs Data_run(height, width);
			FindRuns(bits, width_new, dword_width, Data_run.runs, labelsolver);

			//generate labels
			Generate_on_MSB_First(width_new, Data_run.runs, labelsolver);

			labelsolver.Dealloc();
			delete[] bits;
		}

	private:
		void FindRuns(const uint32_t* bits_start, int width_new, int dword_width, Run* runs, UFPC& labelsolver) {
			Run* runs_up = runs;

			//process runs in the first row
			const uint32_t* bits = bits_start;
			const uint32_t* bit_final = bits + width / 32 + (width % 32 != 0);
			uint32_t working_bits = *bits;
			unsigned long basepos = 0, bitpos = 0;
			for (;; runs++) {
				//find starting position
				while (!myBitScanForward32(&bitpos, working_bits)) {
					bits++, basepos += 32;
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
				working_bits = (~working_bits) & (0xFFFFFFFF << bitpos);
				while (!myBitScanForward32(&bitpos, working_bits)) {
					bits++, basepos += 32;
					if (bits == bit_final) {
						bitpos = 0;
						working_bits = 0xFFFFFFFF;
						break;
					}
					working_bits = ~(*bits);
				}
				working_bits = (~working_bits) & (0xFFFFFFFF << bitpos);
				runs->end_pos = short(basepos + bitpos);
				runs->label = labelsolver.NewLabel();
			}
		out:

			//process runs in the rests
			for (int row = 1; row < height; row++) {
				Run* runs_save = runs;
				const uint32_t* bits = bits_start + dword_width * row;
				const uint32_t* bit_final = bits + width / 32 + (width % 32 != 0);
				uint32_t working_bits = *bits;
				unsigned long basepos = 0, bitpos = 0;
				for (;; runs++) {
					//find starting position
					while (!myBitScanForward32(&bitpos, working_bits)) {
						bits++, basepos += 32;
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
					working_bits = (~working_bits) & (0xFFFFFFFF << bitpos);
					while (!myBitScanForward32(&bitpos, working_bits)) {
						bits++, basepos += 32;
						if (bits == bit_final) {
							bitpos = 0;
							working_bits = 0xFFFFFFFF;
							break;
						}
						working_bits = ~(*bits);
					}
					working_bits = (~working_bits) & (0xFFFFFFFF << bitpos);
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
		void Generate(const Run* runs, UFPC& labelsolver) {
			if (!(fmbits & BTCPR_FM_NO_ZEROINIT)) memset(dest, 0, height * width * sizeof(unsigned));

			if (!(fmbits & BTCPR_FM_NO_ZEROINIT) || (fmbits & BTCPR_FM_ZERO_BUF)) {
				for (int i = 0; i < height; i++) {
					unsigned* labels = dest + width * i;
					for (;; runs++) {
						unsigned short start_pos = runs->start_pos;
						if (start_pos == 0xFFFF) {
							runs++;
							break;
						}
						unsigned short end_pos = runs->end_pos;
						unsigned label = labelsolver.GetLabel(runs->label);
						for (int j = start_pos; j < end_pos; j++) labels[j] = label;
					}
				}
			}
			else {
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
			}
		}
		void Generate_on_MSB_First(int width_new, const Run* runs, UFPC& labelsolver) {
			if (!(fmbits & BTCPR_FM_NO_ZEROINIT)) memset(dest, 0, height * width * sizeof(unsigned));

			if (!(fmbits & BTCPR_FM_NO_ZEROINIT) || (fmbits & BTCPR_FM_ZERO_BUF)) {
				for (int i = 0; i < height; i++) {
					unsigned* labels = dest + width * i;
					for (;; runs++) {
						unsigned short start_pos = width_new - runs->start_pos;
						if (runs->start_pos == 0xFFFF) {
							runs++;
							break;
						}
						unsigned short end_pos = width_new - runs->end_pos;
						unsigned label = labelsolver.GetLabel(runs->label);
						for (int j = start_pos - 1; j >= end_pos; j--) labels[j] = label;
					}
				}
			}
			else {
				for (int i = 0; i < height; i++) {
					unsigned* labels = dest + width * i;
					for (int j = width - 1;; runs++) {
						unsigned short start_pos = width_new - runs->start_pos;
						if (runs->start_pos == 0xFFFF) {
							for (; j >= 0; j--) labels[j] = 0;
							runs++;
							break;
						}
						unsigned short end_pos = width_new - runs->end_pos;
						unsigned label = labelsolver.GetLabel(runs->label);
						for (; j >= start_pos; j--) labels[j] = 0;
						for (; j >= end_pos; j--) labels[j] = label;
					}
				}
			}
		}
	};
	BRTS8(dest, source, height, width, data_width, fmbits).Perform();
};