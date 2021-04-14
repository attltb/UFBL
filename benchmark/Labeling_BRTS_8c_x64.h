#ifndef HEADER_LABELING_BRTS8_X64
#define HEADER_LABELING_BRTS8_X64
#include "Formats.h"
void Labeling_BRTS8_X64(unsigned* dest, const void* source, int height, int width, int data_width = 0, int fmbits = BTCPR_FM_ALIGN_8 | BTCPR_FM_PADDING_ZERO);
#endif