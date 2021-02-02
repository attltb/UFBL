#ifndef YACCLAB_LABELING_ME2
#define YACCLAB_LABELING_ME2
#include <vector>
#include <opencv2/core.hpp>

#include "labeling_algorithms.h"
#include "labels_solver.h"
#include "memory_tester.h"
#include <intrin0.h>

template <typename LabelsSolver>
class BMRS : public Labeling2D<Connectivity2D::CONN_8>
{
    struct Data_Compressed {
        unsigned __int64* bits;
        int height;
        int width;
        int data_width;
        unsigned __int64* operator [](unsigned __int64 row) {
            return bits + data_width * row;
        }
        Data_Compressed(int _height, int _width) : height(_height), width(_width) {
            data_width = _width / 64 + 1;
            bits = new unsigned __int64[height * data_width];
        }
        ~Data_Compressed() {
            delete[] bits;
        }
    };
    struct Run {
        unsigned short start_pos;
        unsigned short end_pos;
        unsigned label;
    };
    struct Runs {
        Run* runs;
        unsigned height;
        unsigned  width;
        Runs(unsigned _height, unsigned _width) : height(_height), width(_width) {
            runs = new Run[height * (width / 2 + 2) + 1];
        };
        ~Runs() {
            delete[] runs;
        }
    };

public:
    BMRS() {}
    void PerformLabeling()
    {
		int w(img_.cols);
		int h(img_.rows);

        Data_Compressed data_compressed(h, w);
        InitCompressedData(data_compressed);

        //generate merged data
        int h_merge = h / 2 + h % 2;
        int data_width = data_compressed.data_width;
        Data_Compressed dt_merged(h_merge, w);
        for (int i = 0; i < h / 2; i++) {
            unsigned __int64* data_source1 = data_compressed[2 * i];
            unsigned __int64* data_source2 = data_compressed[2 * i + 1];
            unsigned __int64* data_merged = dt_merged[i];
            for (int j = 0; j < data_width; j++) data_merged[j] = data_source1[j] | data_source2[j];
        }
        if (h % 2) {
            unsigned __int64* data_source = data_compressed[h - 1];
            unsigned __int64* data_merged = dt_merged[h / 2];
            for (int j = 0; j < data_width; j++) data_merged[j] = data_source[j];
        }

        //generate flag bits
        Data_Compressed dt_flags(h_merge - 1, w);
        for (int i = 0; i < dt_flags.height; i++) {
            unsigned __int64* bits_u = data_compressed[2 * i + 1];
            unsigned __int64* bits_d = data_compressed[2 * i + 2];
            unsigned __int64* bits_dest = dt_flags[i];

            unsigned __int64 u0 = bits_u[0];
            unsigned __int64 d0 = bits_d[0];
            bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
            for (int j = 1; j < data_width; j++) {
                unsigned __int64 u = bits_u[j];
                unsigned __int64 u_shl = u << 1;
                unsigned __int64 d = bits_d[j];
                unsigned __int64 d_shl = d << 1;
                if (bits_u[j - 1] & 0x8000000000000000) u_shl |= 1;
                if (bits_d[j - 1] & 0x8000000000000000) d_shl |= 1;
                bits_dest[j] = (u | u_shl) & (d | d_shl);
            }
        }

        Runs Data_run(h_merge, w);
        Run* run_next = Data_run.runs;
        for (int i = 0; i < h_merge; i++) {
            run_next = FindRuns(dt_merged[i], dt_merged[i + 1], run_next);
        }

        LabelsSolver::Alloc(UPPER_BOUND_8_CONNECTIVITY);
        LabelsSolver::Setup();
        GenerateLabelsOnRun(dt_flags, Data_run.runs, run_next);

        img_labels_ = cv::Mat1i(img_.size());

        Run* runs = Data_run.runs;
        for (int i = 0; i < h / 2; i++) {
            const uchar* const data_u = img_.ptr<uchar>(2 * i);
            const uchar* const data_d = img_.ptr<uchar>(2 * i + 1);
            unsigned* const labels_u = img_labels_.ptr<unsigned>(2 * i);
            unsigned* const labels_d = img_labels_.ptr<unsigned>(2 * i + 1);

            for (int j = 0;; runs++) {
                unsigned short start_pos = runs->start_pos;
                if (start_pos == 0xFFFF) {
                    for (int k = j; k < w; k++) labels_u[k] = 0;
                    for (int k = j; k < w; k++) labels_d[k] = 0;
                    runs++;
                    break;
                }
                unsigned short end_pos = runs->end_pos;
                int label = LabelsSolver::GetLabel(runs->label);

                for (; j < start_pos; j++) labels_u[j] = 0, labels_d[j] = 0;
                for (; j < end_pos; j++) {
                    labels_u[j] = (data_u[j]) ? label : 0;
                    labels_d[j] = (data_d[j]) ? label : 0;
                }
            }
        }
        if (h % 2) {
            unsigned* const labels = img_labels_.ptr<unsigned>(h - 1);
            for (int j = 0;; runs++) {
                unsigned short start_pos = runs->start_pos;
                if (start_pos == 0xFFFF) {
                    for (int k = j; k < w; k++) labels[k] = 0;
                    break;
                }
                unsigned short end_pos = runs->end_pos;
                int label = LabelsSolver::GetLabel(runs->label);
                for (; j < start_pos; j++) labels[j] = 0;
                for (j = start_pos; j < end_pos; j++) labels[j] = label;
            }
        }

        LabelsSolver::Dealloc();
    }

private:
    void InitCompressedData(Data_Compressed& data_compressed) {
        int w(img_.cols);
        int h(img_.rows);

        for (int i = 0; i < h; i++) {
            unsigned __int64* mbits = data_compressed[i];
            uchar* source = img_.ptr<uchar>(i);
            for (int j = 0; j < w >> 6; j++) {
                uchar* base = source + (j << 6);
                unsigned __int64 obits = 0;
                if (base[0]) obits |= 0x0000000000000001;
                if (base[1]) obits |= 0x0000000000000002;
                if (base[2]) obits |= 0x0000000000000004;
                if (base[3]) obits |= 0x0000000000000008;
                if (base[4]) obits |= 0x0000000000000010;
                if (base[5]) obits |= 0x0000000000000020;
                if (base[6]) obits |= 0x0000000000000040;
                if (base[7]) obits |= 0x0000000000000080;
                if (base[8]) obits |= 0x0000000000000100;
                if (base[9]) obits |= 0x0000000000000200;
                if (base[10]) obits |= 0x0000000000000400;
                if (base[11]) obits |= 0x0000000000000800;
                if (base[12]) obits |= 0x0000000000001000;
                if (base[13]) obits |= 0x0000000000002000;
                if (base[14]) obits |= 0x0000000000004000;
                if (base[15]) obits |= 0x0000000000008000;
                if (base[16]) obits |= 0x0000000000010000;
                if (base[17]) obits |= 0x0000000000020000;
                if (base[18]) obits |= 0x0000000000040000;
                if (base[19]) obits |= 0x0000000000080000;
                if (base[20]) obits |= 0x0000000000100000;
                if (base[21]) obits |= 0x0000000000200000;
                if (base[22]) obits |= 0x0000000000400000;
                if (base[23]) obits |= 0x0000000000800000;
                if (base[24]) obits |= 0x0000000001000000;
                if (base[25]) obits |= 0x0000000002000000;
                if (base[26]) obits |= 0x0000000004000000;
                if (base[27]) obits |= 0x0000000008000000;
                if (base[28]) obits |= 0x0000000010000000;
                if (base[29]) obits |= 0x0000000020000000;
                if (base[30]) obits |= 0x0000000040000000;
                if (base[31]) obits |= 0x0000000080000000;
                if (base[32]) obits |= 0x0000000100000000;
                if (base[33]) obits |= 0x0000000200000000;
                if (base[34]) obits |= 0x0000000400000000;
                if (base[35]) obits |= 0x0000000800000000;
                if (base[36]) obits |= 0x0000001000000000;
                if (base[37]) obits |= 0x0000002000000000;
                if (base[38]) obits |= 0x0000004000000000;
                if (base[39]) obits |= 0x0000008000000000;
                if (base[40]) obits |= 0x0000010000000000;
                if (base[41]) obits |= 0x0000020000000000;
                if (base[42]) obits |= 0x0000040000000000;
                if (base[43]) obits |= 0x0000080000000000;
                if (base[44]) obits |= 0x0000100000000000;
                if (base[45]) obits |= 0x0000200000000000;
                if (base[46]) obits |= 0x0000400000000000;
                if (base[47]) obits |= 0x0000800000000000;
                if (base[48]) obits |= 0x0001000000000000;
                if (base[49]) obits |= 0x0002000000000000;
                if (base[50]) obits |= 0x0004000000000000;
                if (base[51]) obits |= 0x0008000000000000;
                if (base[52]) obits |= 0x0010000000000000;
                if (base[53]) obits |= 0x0020000000000000;
                if (base[54]) obits |= 0x0040000000000000;
                if (base[55]) obits |= 0x0080000000000000;
                if (base[56]) obits |= 0x0100000000000000;
                if (base[57]) obits |= 0x0200000000000000;
                if (base[58]) obits |= 0x0400000000000000;
                if (base[59]) obits |= 0x0800000000000000;
                if (base[60]) obits |= 0x1000000000000000;
                if (base[61]) obits |= 0x2000000000000000;
                if (base[62]) obits |= 0x4000000000000000;
                if (base[63]) obits |= 0x8000000000000000;
                *mbits = obits, mbits++;
            }
            unsigned __int64 obits_final = 0;
            int jbase = w - (w % 64);
            for (int j = 0; j < w % 64; j++) {
                if (source[jbase + j]) obits_final |= ((__int64)1 << j);
            }
            *mbits = obits_final;
        }
    }
    Run* FindRuns(unsigned __int64* bits, unsigned __int64* bits_max, Run* runs) {
        unsigned __int64 working_bits = *bits;
        unsigned long basepos = 0, bitpos = 0;
        for (;; runs++) {
            //find starting position
            while (!_BitScanForward64(&bitpos, working_bits)) {
                bits++, basepos += 64;
                if (bits == bits_max) {
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
                working_bits = ~(*bits);
            }
            working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
            runs->end_pos = short(basepos + bitpos);
        }
    }
    void GenerateLabelsOnRun(const Data_Compressed& dt_flags, Run* runs, Run* runs_end) {
        Run* runs_up = runs;

        //generate labels for the first row
        for (; runs->start_pos != 0xFFFF; runs++) runs->label = LabelsSolver::NewLabel();
        runs++;

        //generate labels for the rests
        int data_width = dt_flags.data_width;
        const unsigned __int64* ibits = dt_flags.bits;
        for (; runs != runs_end; runs++) {
            Run* runs_save = runs;
            for (;; runs++) {
                unsigned short start_pos = runs->start_pos;
                if (start_pos == 0xFFFF) break;

                //Skip upper runs end before this slice starts
                for (; runs_up->end_pos < start_pos; runs_up++);

                //No upper run meets this
                unsigned short end_pos = runs->end_pos;
                if (runs_up->start_pos > end_pos) {
                    runs->label = LabelsSolver::NewLabel();
                    continue;
                };

                //Next upper run can not meet this
                unsigned short cross_st = (start_pos >= runs_up->start_pos) ? start_pos : runs_up->start_pos;
                if (end_pos <= runs_up->end_pos) {
                    if (is_connected(ibits, cross_st, end_pos)) runs->label = LabelsSolver::GetLabel(runs_up->label);
                    else runs->label = LabelsSolver::NewLabel();
                    continue;
                }

                unsigned label;
                if (is_connected(ibits, cross_st, runs_up->end_pos)) label = LabelsSolver::GetLabel(runs_up->label);
                else label = 0;
                runs_up++;

                //Find next upper runs meet this
                for (; runs_up->start_pos <= end_pos; runs_up++) {
                    if (end_pos <= runs_up->end_pos) {
                        if (is_connected(ibits, runs_up->start_pos, end_pos)) {
                            unsigned label_other = LabelsSolver::GetLabel(runs_up->label);
                            if (label != label_other) {
                                label = (label) ? LabelsSolver::Merge(label, label_other) : label_other;
                            }
                        }
                        break;
                    }
                    else {
                        if (is_connected(ibits, runs_up->start_pos, runs_up->end_pos)) {
                            unsigned label_other = LabelsSolver::GetLabel(runs_up->label);
                            if (label != label_other) {
                                label = (label) ? LabelsSolver::Merge(label, label_other) : label_other;
                            }
                        }
                    }
                }

                if (label) runs->label = label;
                else runs->label = LabelsSolver::NewLabel();
            }
            runs_up = runs_save;
            ibits += data_width;
        }
        n_labels_ = LabelsSolver::Flatten();
    }
    unsigned __int64 is_connected(const unsigned __int64* flag_bits, unsigned start, unsigned end) {
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
};
#endif 