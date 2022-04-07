#ifndef HEADER_FORMATS
#define HEADER_FORMATS
constexpr auto BTCPR_FM_B1W0 = 0x80000000;
constexpr auto BTCPR_FM_MSB_FIRST = 0x40000000;
constexpr auto BTCPR_FM_PADDING_MSB = 0x20000000;
constexpr auto BTCPR_FM_PADDING_ZERO = 0x10000000;
constexpr auto BTCPR_FM_ZERO_BUF = 0x08000000;
constexpr auto BTCPR_FM_NO_ZEROINIT = 0x04000000;

constexpr auto BTCPR_FM_NO_ALIGN = 0x00000000;
constexpr auto BTCPR_FM_ALIGN_1 = 0x00000001;
constexpr auto BTCPR_FM_ALIGN_2 = 0x00000002;
constexpr auto BTCPR_FM_ALIGN_4 = 0x00000004;
constexpr auto BTCPR_FM_ALIGN_8 = 0x00000008;

constexpr auto BTCPR_FM_PBM4 = BTCPR_FM_B1W0 | BTCPR_FM_MSB_FIRST | BTCPR_FM_ALIGN_1;
constexpr auto BTCPR_FM_ALIGNED = BTCPR_FM_ALIGN_1 | BTCPR_FM_ALIGN_2 | BTCPR_FM_ALIGN_4 | BTCPR_FM_ALIGN_8;

inline bool GetBit(const void* bits, int height, int width, int data_width, int fmbits, int y, int x) {
	const unsigned char* m_bits = (const unsigned char*)bits;
	if (!data_width) {
		if (fmbits & BTCPR_FM_ALIGN_1) data_width = (width / 8) + ((width % 8) ? 1 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_2) data_width = (width / 16) * 2 + ((width % 16) ? 2 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_4) data_width = (width / 32) * 4 + ((width % 32) ? 4 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_8) data_width = (width / 64) * 8 + ((width % 64) ? 8 : 0);
	}

	if (data_width) {
		int index = 8 * data_width * y + x;
		int byteindex = index / 8;
		int bitindex = index % 8;

		int bitno;
		if (fmbits & BTCPR_FM_MSB_FIRST) {
			if (fmbits & BTCPR_FM_PADDING_MSB) {
				if (width < (x - (x % 8)) + 8) bitno = (width - (x - (x % 8)) - 1) - bitindex;
				else bitno = 7 - bitindex;
			}
			else bitno = 7 - bitindex;
		}
		else bitno = bitindex;

		if (fmbits & BTCPR_FM_B1W0) {
			if (m_bits[byteindex] & (1 << bitno)) return false;
			else return true;
		}
		else {
			if (m_bits[byteindex] & (1 << bitno)) return true;
			else return false;
		}
	}
	else {
		int size = height * width;
		int index = width * y + x;
		int byteindex = index / 8;
		int bitindex = index % 8;

		int bitno;
		if (fmbits & BTCPR_FM_MSB_FIRST) {
			if (fmbits & BTCPR_FM_PADDING_MSB) {
				if (size < (byteindex * 8) + 8) bitno = (size - (byteindex * 8) - 1) - bitindex;
				else bitno = 7 - bitindex;
			}
			else bitno = 7 - bitindex;
		}
		else bitno = bitindex;

		if (fmbits & BTCPR_FM_B1W0) {
			if (m_bits[byteindex] & (1 << bitno)) return false;
			else return true;
		}
		else {
			if (m_bits[byteindex] & (1 << bitno)) return true;
			else return false;
		}
	}
}
#endif