#ifndef HEADER_LABELING_BRTS4_X64
#define HEADER_LABELING_BRTS4_X64
#include "Formats.h"
void Labeling_BRTS4_X64(unsigned* dest, const void* source, int height, int width, int data_width = 0, int fmbits = BTCPR_FM_ALIGN_8 | BTCPR_FM_PADDING_ZERO);
void Labeling_BRTS4_on_byte_X64(unsigned* dest, const uint8_t* source, int height, int width, int data_width, int fmbits = 0);
#endif