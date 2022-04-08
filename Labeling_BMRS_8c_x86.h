#ifndef HEADER_LABELING_BMRS_X86
#define HEADER_LABELING_BMRS_X86
#include "Formats.h"
void Labeling_BMRS_X86(unsigned* dest, const void* source, int height, int width, int data_width = 0, int fmbits = BTCPR_FM_ALIGN_4 | BTCPR_FM_PADDING_ZERO);
void Labeling_BMRS_on_byte_X86(unsigned* dest, const uint8_t* source, int height, int width, int data_width, int fmbits = 0);
#endif