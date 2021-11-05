#ifndef HEADER_PRINTER
#define HEADER_PRINTER
char GetLabel(unsigned label);
void PrintLabel(const unsigned int* labels, int height, int width, bool space = true);

void PrintData(const unsigned int* source, int height, int width, bool space = true);
void PrintCompressedBits_X86(uint32_t bits, bool space = true);
void PrintCompressedBits_X64(uint64_t bits, bool space = true);
void PrintCompressedBits_on_MsbFirst_X86(uint32_t bits, bool space = true);
void PrintCompressedBits_on_MsbFirst_X64(uint64_t bits, bool space = true);
void PrintCompressedBits(const void* bits, int height, int width, int data_width, int fmbits, int reverse = 0, bool space = true);
#endif