#ifndef YACCLAB_LABELING_ME1
#define YACCLAB_LABELING_ME1
#include <vector>
#include <opencv2/core.hpp>

#include "labeling_algorithms.h"
#include "labels_solver.h"
#include "memory_tester.h"
#include <intrin0.h>

template <typename LabelsSolver>
class BRTS : public Labeling2D<Connectivity2D::CONN_8>
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
    BRTS() {}
    void PerformLabeling()
    {
        img_labels_ = cv::Mat1i(img_.size());

        int w(img_.cols);
        int h(img_.rows);

        Data_Compressed data_compressed(h, w);
        InitCompressedData(data_compressed);

        Runs Data_run(h, w);
        Run* run_next = Data_run.runs;
        for (int i = 0; i < h; i++) {
            run_next = FindRuns(data_compressed[i], data_compressed[i + 1], run_next);
        }

        LabelsSolver::Alloc(UPPER_BOUND_8_CONNECTIVITY);
        LabelsSolver::Setup();
        GenerateLabelsOnRun(Data_run.runs, run_next);

        Run* runs = Data_run.runs;
        for (int i = 0; i < h; i++) {
            unsigned* const labels = img_labels_.ptr<unsigned>(i);
            for (int j = 0;; runs++) {
                unsigned short start_pos = runs->start_pos;
                if (start_pos == 0xFFFF) {
                    for (; j < w; j++) labels[j] = 0;
                    runs++;
                    break;
                }
                unsigned short end_pos = runs->end_pos;
                int label = LabelsSolver::GetLabel(runs->label);
                for (; j < start_pos; j++) labels[j] = 0;
                for (; j < end_pos; j++) labels[j] = label;
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
    void GenerateLabelsOnRun(Run* runs, Run* runs_end) {
        Run* runs_up = runs;

        //generate labels for the first row
        for (; runs->start_pos != 0xFFFF; runs++) runs->label = LabelsSolver::NewLabel();
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
                    runs->label = LabelsSolver::NewLabel();
                    continue;
                };

                unsigned label = LabelsSolver::GetLabel(runs_up->label);

                //Next upper run can not meet this
                if (end_pos <= runs_up->end_pos) {
                    runs->label = label;
                    continue;
                }

                //Find next upper runs meet this
                runs_up++;
                for (; runs_up->start_pos <= end_pos; runs_up++) {
                    unsigned label_other = LabelsSolver::GetLabel(runs_up->label);
                    if (label != label_other) label = LabelsSolver::Merge(label, label_other);
                    if (end_pos <= runs_up->end_pos) break;
                }
                runs->label = label;
            }
            runs_up = runs_save;
        }
        n_labels_ = LabelsSolver::Flatten();
    }
};
#endif 