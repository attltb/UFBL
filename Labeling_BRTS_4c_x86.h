#ifndef HEADER_LABELING_BRTS4_X86
#define HEADER_LABELING_BRTS4_X86
#include "Formats.h"
void Labeling_BRTS4_X86(unsigned* dest, const void* source, int height, int width, int data_width = 0, int fmbits = BTCPR_FM_ALIGN_4 | BTCPR_FM_PADDING_ZERO);
#endif