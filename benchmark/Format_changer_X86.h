#ifndef HEADER_FORMATCHANGER_X86
#define HEADER_FORMATCHANGER_X86
#include "Formats.h"
#include <stdint.h>
#include <utility>
#include <string.h>
#include <stdlib.h>
inline std::pair<const uint32_t*, std::pair<int, int>> CCL_Bswap_X86(const void* source, int height, int width, int data_width_src, int fmbits) {
	int dword_width_dst = width / 32;
	uint32_t* dest = new uint32_t[height * dword_width_dst];
	uint32_t* m_dst = dest;
	const unsigned char* m_src_c = (const unsigned char*)source + (dword_width_dst - 1) * 4;

	if (fmbits & BTCPR_FM_B1W0) {
		for (int i = 0; i < height; i++) {
			const uint32_t* m_src = (const uint32_t*)m_src_c;
			uint32_t* m_dst_end = m_dst + dword_width_dst;
			for (; m_dst < m_dst_end; m_dst++, m_src--) *m_dst = ~_byteswap_ulong(*m_src);
			m_src_c += data_width_src;
		}
	}
	else {
		for (int i = 0; i < height; i++) {
			const uint32_t* m_src = (const uint32_t*)m_src_c;
			uint32_t* m_dst_end = m_dst + dword_width_dst;
			for (; m_dst < m_dst_end; m_dst++, m_src--) *m_dst = _byteswap_ulong(*m_src);
			m_src_c += data_width_src;
		}
	}
	return std::make_pair(dest, std::make_pair(dword_width_dst, width));
}
inline std::pair<const uint32_t*, std::pair<int, int>> CCL_Add_Padding_and_Bswap_X86(const void* source, int height, int width, int fmbits) {
	if (width < 32) {
		int size = height * width;
		int size_div_8 = size / 8;
		int size_rem_8 = size % 8;
		int size_div_32 = size / 32;
		int size_rem_32 = size % 32;

		uint32_t* dest = new uint32_t[height];
		uint32_t* m_dst = dest + (height - 1);

		uint32_t bits = 0;
		int bitlen = size_rem_32;
		uint32_t bits_mask = ~(0xFFFFFFFF << width);
		if (size_rem_32) {
			const unsigned char* p_srcc = (const unsigned char*)source + size_div_8;

			if (fmbits & BTCPR_FM_B1W0) {
				if (size_rem_8) {
					if (fmbits & BTCPR_FM_PADDING_MSB) bits = (uint32_t)(unsigned char)(~p_srcc[0]) & ~(0xFFFFFFFF << size_rem_8);
					else bits = (uint32_t)(unsigned char)(~p_srcc[0]) >> (8 - size_rem_8);
				}
				if (8 * 1 <= size_rem_32) bits |= ((uint32_t)(unsigned char)(~p_srcc[-1]) << (size_rem_8));
				if (8 * 2 <= size_rem_32) bits |= ((uint32_t)(unsigned char)(~p_srcc[-2]) << (size_rem_8 + 8 * 1));
				if (8 * 3 <= size_rem_32) bits |= ((uint32_t)(unsigned char)(~p_srcc[-3]) << (size_rem_8 + 8 * 2));

				for (; bitlen >= width;) {
					*m_dst = bits & bits_mask, m_dst--;
					bitlen -= width, bits >>= width;
				}
			}
			else {
				if (size_rem_8) {
					if (fmbits & BTCPR_FM_PADDING_MSB) bits = (uint32_t)(p_srcc[0]) & ~(0xFFFFFFFF << size_rem_8);
					else bits = (uint32_t)(p_srcc[0]) >> (8 - size_rem_8);
				}
				if (8 * 1 <= size_rem_32) bits |= ((uint32_t)(p_srcc[-1]) << (size_rem_8));
				if (8 * 2 <= size_rem_32) bits |= ((uint32_t)(p_srcc[-2]) << (size_rem_8 + 8 * 1));
				if (8 * 3 <= size_rem_32) bits |= ((uint32_t)(p_srcc[-3]) << (size_rem_8 + 8 * 2));

				for (; bitlen >= width;) {
					*m_dst = bits & bits_mask, m_dst--;
					bitlen -= width, bits >>= width;
				}
			}
		}

		int bitlen_to_fill = width - bitlen;
		const uint32_t* m_src = (const uint32_t*)source + size_div_32 - 1;
		if (fmbits & BTCPR_FM_B1W0) {
			for (; source <= m_src; m_src--) {
				uint32_t bits_n = ~_byteswap_ulong(*m_src);

				bits |= (bits_n << bitlen);
				bits &= bits_mask;
				*m_dst = bits, m_dst--;

				int bitlen_remain = 32 - bitlen_to_fill;
				bits_n >>= bitlen_to_fill;
				for (; width <= bitlen_remain;) {
					*m_dst = bits_n & bits_mask, m_dst--;
					bitlen_remain -= width, bits_n >>= width;
				}

				bits = bits_n;
				bitlen = bitlen_remain;
				bitlen_to_fill = width - bitlen;
			}
		}
		else {
			for (; source <= m_src; m_src--) {
				uint32_t bits_n = _byteswap_ulong(*m_src);

				bits |= (bits_n << bitlen);
				bits &= bits_mask;
				*m_dst = bits, m_dst--;

				int bitlen_remain = 32 - bitlen_to_fill;
				bits_n >>= bitlen_to_fill;
				for (; width <= bitlen_remain;) {
					*m_dst = bits_n & bits_mask, m_dst--;
					bitlen_remain -= width, bits_n >>= width;
				}

				bits = bits_n;
				bitlen = bitlen_remain;
				bitlen_to_fill = width - bitlen;
			}
		}
		return std::make_pair(dest, std::make_pair(1, width));
	}

	int size = height * width;
	int size_div_8 = size / 8;
	int size_rem_8 = size % 8;
	int size_div_32 = size / 32;
	int size_rem_32 = size % 32;

	int width_div_32 = width / 32;
	int width_rem_32 = width % 32;
	int dword_width = width_div_32 + 1;
	uint32_t* dest = new uint32_t[height * dword_width];
	uint32_t* m_dst = dest + (height - 1) * dword_width;

	uint32_t bits = 0;
	int bitlen = size_rem_32;
	int bitlen_r = 32 - size_rem_32;
	uint32_t bits_mask_edge = ~(0xFFFFFFFF << width_rem_32);
	if (size_rem_32) {
		const unsigned char* p_srcc = (const unsigned char*)source + size_div_8;
		if (fmbits & BTCPR_FM_B1W0) {
			if (size_rem_8) {
				if (fmbits & BTCPR_FM_PADDING_MSB) bits = (uint32_t)(unsigned char)(~p_srcc[0]) & ~(0xFFFFFFFF << size_rem_8);
				else bits = (uint32_t)(unsigned char)(~p_srcc[0]) >> (8 - (size_rem_8));
			}
			if (8 * 1 <= size_rem_32) bits |= ((uint32_t)(unsigned char)(~p_srcc[-1]) << (size_rem_8));
			if (8 * 2 <= size_rem_32) bits |= ((uint32_t)(unsigned char)(~p_srcc[-2]) << (size_rem_8 + 8 * 1));
			if (8 * 3 <= size_rem_32) bits |= ((uint32_t)(unsigned char)(~p_srcc[-3]) << (size_rem_8 + 8 * 2));
		}
		else {
			if (size_rem_8) {
				if (fmbits & BTCPR_FM_PADDING_MSB) bits = (uint32_t)(p_srcc[0]) & ~(0xFFFFFFFF << size_rem_8);
				else bits = (uint32_t)(p_srcc[0]) >> (8 - (size_rem_8));
			}
			if (8 * 1 <= size_rem_32) bits |= ((uint32_t)(p_srcc[-1]) << (size_rem_8));
			if (8 * 2 <= size_rem_32) bits |= ((uint32_t)(p_srcc[-2]) << (size_rem_8 + 8 * 1));
			if (8 * 3 <= size_rem_32) bits |= ((uint32_t)(p_srcc[-3]) << (size_rem_8 + 8 * 2));
		}
	}

	const uint32_t* m_src = (const uint32_t*)source + size_div_32 - 1;
	for (;;) {
		if (!bitlen) {
			uint32_t* m_dst_e = m_dst + width / 32;

			if (fmbits & BTCPR_FM_B1W0) {
				for (; m_dst < m_dst_e; m_src--) {
					*m_dst = ~_byteswap_ulong(*m_src), m_dst++;
				}
			}
			else {
				for (; m_dst < m_dst_e; m_src--) {
					*m_dst = _byteswap_ulong(*m_src), m_dst++;
				}
			}

			int bitlen_rem = width % 32;
			uint32_t bits_n = (fmbits & BTCPR_FM_B1W0) ? ~_byteswap_ulong(*m_src) : _byteswap_ulong(*m_src);
			m_src--;

			*m_dst = bits_n & bits_mask_edge;
			m_dst -= dword_width * 2 - 1;

			bitlen_r = bitlen_rem;
			bitlen = 32 - bitlen_rem;
			bits = bits_n >> bitlen_r;
		}
		else {
			int width_rem = width - bitlen;
			uint32_t* m_dst_e = m_dst + width_rem / 32;

			if (fmbits & BTCPR_FM_B1W0) {
				for (; m_dst < m_dst_e; m_src--) {
					uint32_t bits_n = ~_byteswap_ulong(*m_src);
					*m_dst = bits | (bits_n << bitlen), m_dst++;
					bits = bits_n >> bitlen_r;
				}
			}
			else {
				for (; m_dst < m_dst_e; m_src--) {
					uint32_t bits_n = _byteswap_ulong(*m_src);
					*m_dst = bits | (bits_n << bitlen), m_dst++;
					bits = bits_n >> bitlen_r;
				}
			}

			int bitlen_rem = width_rem % 32;
			if (!bitlen_rem) {
				*m_dst = bits;
				m_dst -= dword_width * 2 - 1;
				if (source > m_src) break;
				bitlen = 0;
			}
			else {
				uint32_t bits_n = (fmbits & BTCPR_FM_B1W0) ? ~_byteswap_ulong(*m_src) : _byteswap_ulong(*m_src);
				m_src--;

				if (bitlen_rem <= bitlen_r) {
					*m_dst = (bits | (bits_n << bitlen)) & bits_mask_edge;
					m_dst -= dword_width * 2 - 1;
				}
				else {
					*m_dst = bits | (bits_n << bitlen);
					*(m_dst + 1) = (bits_n >> bitlen_r) & bits_mask_edge;
					m_dst -= dword_width * 2 - 2;
				}

				bitlen_r = bitlen_rem;
				bitlen = 32 - bitlen_rem;
				bits = bits_n >> bitlen_r;
			}
		}
	}
	return std::make_pair(dest, std::make_pair(dword_width, width));
}
inline std::pair<const uint32_t*, std::pair<int, int>> CCL_Modify_Padding_and_Bswap_X86(const void* source, int height, int width, int data_width_src, int fmbits) {
	int width_div_32 = width / 32;
	int width_rem_32 = width % 32;

	uint32_t bits_mask_start = ~(0xFFFFFFFF >> width_rem_32);
	int dword_width_full = width_div_32;
	int dword_width = dword_width_full + (width_rem_32 != 0);
	uint32_t* dest = new uint32_t[height * dword_width];
	uint32_t* m_dst = dest;
	uint32_t* m_dst_end = dest + height * dword_width;
	const unsigned char* m_src_c = (const unsigned char*)source;

	if (fmbits & BTCPR_FM_B1W0) {
		for (; m_dst < m_dst_end; m_src_c += data_width_src) {
			const unsigned char* m_src_edge = m_src_c + (dword_width - 1) * sizeof(long);
			if (width_rem_32) {
				uint32_t last_val = 0;
				int i = 0;
				for (; i < width_rem_32 / 8; i++) last_val |= (uint32_t)(m_src_edge[i]) << (8 * i);
				if (width_rem_32 % 8) {
					uint32_t last_last_val = (uint32_t)(m_src_edge[i]) << (8 * i);
					if (fmbits & BTCPR_FM_PADDING_MSB) last_last_val <<= (8 - (width % 8));
					last_val |= last_last_val;
				}
				*m_dst = ~_byteswap_ulong(last_val) & bits_mask_start, m_dst++, m_src_edge -= sizeof(long);
			}

			uint32_t* m_dst_end = m_dst + width_div_32;
			const uint32_t* m_src = (const uint32_t*)m_src_edge;
			for (; m_dst < m_dst_end; m_dst++, m_src--) *m_dst = ~_byteswap_ulong(*m_src);
		}
	}
	else {
		for (; m_dst < m_dst_end; m_src_c += data_width_src) {
			const unsigned char* m_src_edge = m_src_c + (dword_width - 1) * sizeof(long);
			if (width_rem_32) {
				uint32_t last_val = 0;
				int i = 0;
				for (; i < width_rem_32 / 8; i++) last_val |= (uint32_t)(m_src_edge[i]) << (8 * i);
				if (width_rem_32 % 8) {
					uint32_t last_last_val = (uint32_t)(m_src_edge[i]) << (8 * i);
					if (fmbits & BTCPR_FM_PADDING_MSB) last_last_val <<= (8 - (width % 8));
					last_val |= last_last_val;
				}
				*m_dst = _byteswap_ulong(last_val) & bits_mask_start, m_dst++, m_src_edge -= sizeof(long);
			}

			uint32_t* m_dst_end = m_dst + width_div_32;
			const uint32_t* m_src = (const uint32_t*)m_src_edge;
			for (; m_dst < m_dst_end; m_dst++, m_src--) *m_dst = _byteswap_ulong(*m_src);
		}
	}
	return std::make_pair(dest, std::make_pair(dword_width, dword_width * 32));
}
inline std::pair<const uint32_t*, std::pair<int, int>> CCL_Format_Change_and_Bswap_X86(const void* source, int height, int width, int data_width, int fmbits) {
	if (!data_width) {
		if (fmbits & BTCPR_FM_ALIGN_1) data_width = (width / 8) + ((width % 8) ? 1 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_2) data_width = (width / 16) * 2 + ((width % 16) ? 2 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_4) data_width = (width / 32) * 4 + ((width % 32) ? 4 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_8) data_width = (width / 64) * 8 + ((width % 64) ? 8 : 0);
		else if (width % 64) return CCL_Add_Padding_and_Bswap_X86(source, height, width, fmbits);
		else data_width = width / 8;
	}

	if (!(width % 32)) return CCL_Bswap_X86(source, height, width, data_width, fmbits);
	return CCL_Modify_Padding_and_Bswap_X86(source, height, width, data_width, fmbits);
}

inline std::pair<const uint32_t*, int> CCL_Bit_Invert_X86(const void* source, int height, int width, int data_width_src, int fmbits) {
	int dword_width_dst = width / 32;
	uint32_t* dest = new uint32_t[height * dword_width_dst];
	uint32_t* m_dst = dest;
	const unsigned char* m_src_c = (const unsigned char*)source;

	for (int i = 0; i < height; i++) {
		const uint32_t* m_src = (const uint32_t*)m_src_c;

		for (int j = 0; j < dword_width_dst; j++) m_dst[j] = ~m_src[j];
		m_dst += dword_width_dst;
		m_src_c += data_width_src;
	}
	return std::make_pair(dest, dword_width_dst);
}
inline std::pair<const uint32_t*, int> CCL_Clear_Padding_X86(const void* source, int height, int width, int dword_width_src, int fmbits) {
	int dword_width_full = width / 32;
	int dword_width_dst = dword_width_full + 1;
	uint32_t bits_mask_edge = ~(0xFFFFFFFF << (width % 32));

	uint32_t* dest = new uint32_t[height * dword_width_dst];
	uint32_t* m_dst = dest;
	const uint32_t* m_src = (const uint32_t*)source;

	if (fmbits & BTCPR_FM_B1W0) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < dword_width_dst; j++) m_dst[j] = ~m_src[j];
			m_dst[dword_width_full] &= bits_mask_edge;
			m_dst += dword_width_dst;
			m_src += dword_width_src;
		}
	}
	else {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < dword_width_dst; j++) m_dst[j] = m_src[j];
			m_dst[dword_width_full] &= bits_mask_edge;
			m_dst += dword_width_dst;
			m_src += dword_width_src;
		}
	}
	return std::make_pair(dest, dword_width_dst);
}
inline std::pair<const uint32_t*, int> CCL_Add_Padding_X86(const void* source, int height, int width, int fmbits) {
	if (width < 32) {
		int size = height * width;
		int size_div_32 = size / 32;
		int size_rem_32 = size % 32;

		uint32_t* dest = new uint32_t[height];
		uint32_t* m_dst = dest;
		uint32_t* m_dst_last = dest + height;
		const uint32_t* m_src = (const uint32_t*)source;
		const uint32_t* m_src_last = m_src + size_div_32;

		uint32_t bits = 0;
		int bitlen_filled = 0;
		int bitlen_to_fill = width;
		uint32_t bits_mask = ~(0xFFFFFFFF << width);

		if (fmbits & BTCPR_FM_B1W0) {
			for (; m_src < m_src_last; m_src++) {
				uint32_t bits_n = ~*m_src;

				bits |= (bits_n << bitlen_filled);
				bits &= bits_mask;
				*m_dst = bits, m_dst++;

				int bitlen_remain = 32 - bitlen_to_fill;
				bits_n >>= bitlen_to_fill;
				for (; width < bitlen_remain;) {
					*m_dst = bits_n & bits_mask, m_dst++;
					bitlen_remain -= width, bits_n >>= width;
				}

				bits = bits_n;
				bitlen_filled = bitlen_remain;
				bitlen_to_fill = width - bitlen_remain;
			}
		}
		else {
			for (; m_src < m_src_last; m_src++) {
				uint32_t bits_n = *m_src;

				bits |= (bits_n << bitlen_filled);
				bits &= bits_mask;
				*m_dst = bits, m_dst++;

				int bitlen_remain = 32 - bitlen_to_fill;
				bits_n >>= bitlen_to_fill;
				for (; width < bitlen_remain;) {
					*m_dst = bits_n & bits_mask, m_dst++;
					bitlen_remain -= width, bits_n >>= width;
				}

				bits = bits_n;
				bitlen_filled = bitlen_remain;
				bitlen_to_fill = width - bitlen_remain;
			}
		}

		const unsigned char* p_srcc = (const unsigned char*)m_src;
		int bitlen_last = size_rem_32;
		uint32_t bits_last = (uint32_t)(p_srcc[0]);
		if (8 * 1 < bitlen_last) bits_last |= ((uint32_t)(p_srcc[1]) << (8 * 1));
		if (8 * 2 < bitlen_last) bits_last |= ((uint32_t)(p_srcc[2]) << (8 * 2));
		if (8 * 3 < bitlen_last) bits_last |= ((uint32_t)(p_srcc[3]) << (8 * 3));
		if (fmbits & BTCPR_FM_B1W0) bits_last = ~bits_last;
		bits_last &= ~(0xFFFFFFFF << bitlen_last);

		bits |= (bits_last << bitlen_filled);
		bits &= bits_mask;
		*m_dst = bits & bits_mask, m_dst++;

		bits_last >>= bitlen_to_fill, bitlen_last -= bitlen_to_fill;
		for (; bitlen_last; bitlen_last -= width, bits_last >>= width) {
			*m_dst = bits_last & bits_mask, m_dst++;
		}
		return std::make_pair(dest, 1);
	}

	int size = height * width;
	int size_div_32 = size / 32;
	int size_rem_32 = size % 32;

	int width_div_32 = width / 32;
	int width_rem_32 = width % 32;
	int dword_width = width_div_32 + 1;
	uint32_t* dest = new uint32_t[height * dword_width];

	uint32_t* m_dst = dest;
	uint32_t* m_dst_last = dest + height * dword_width;
	const uint32_t* m_src = (const uint32_t*)source;
	const uint32_t* m_src_last = m_src + size_div_32;

	uint32_t bits = 0;
	int bitlen = 0;
	int bitlen_r = 32;
	uint32_t bits_mask_edge = ~(0xFFFFFFFF << width_rem_32);

	for (;;) {
		if (!bitlen) {
			uint32_t* m_dst_e = m_dst + width / 32;
			if (fmbits & BTCPR_FM_B1W0) {
				for (; m_dst < m_dst_e; m_src++) *m_dst = ~*m_src, m_dst++;
			}
			else {
				for (; m_dst < m_dst_e; m_src++) *m_dst = *m_src, m_dst++;
			}

			int bitlen_rem = width % 32;
			if (m_src_last == m_src) break;

			uint32_t bits_n = (fmbits & BTCPR_FM_B1W0) ? ~*m_src : *m_src;
			m_src++;

			*m_dst = bits_n & bits_mask_edge, m_dst++;
			bitlen_r = bitlen_rem;
			bitlen = 32 - bitlen_rem;
			bits = bits_n >> bitlen_r;
		}
		else {
			int width_rem = width - bitlen;
			uint32_t* m_dst_e = m_dst + width_rem / 32;

			if (fmbits & BTCPR_FM_B1W0) {
				for (; m_dst < m_dst_e; m_src++) {
					uint32_t bits_n = ~*m_src;
					*m_dst = bits | (bits_n << bitlen), m_dst++;
					bits = bits_n >> bitlen_r;
				}
			}
			else {
				for (; m_dst < m_dst_e; m_src++) {
					uint32_t bits_n = *m_src;
					*m_dst = bits | (bits_n << bitlen), m_dst++;
					bits = bits_n >> bitlen_r;
				}
			}

			int bitlen_rem = width_rem % 32;
			if (!bitlen_rem) {
				*m_dst = bits, m_dst++;
				if (m_src_last == m_src) break;
				bitlen = 0;
			}
			else {
				if (m_src_last == m_src) break;
				uint32_t bits_n = (fmbits & BTCPR_FM_B1W0) ? ~*m_src : *m_src;
				m_src++;

				if (bitlen_rem <= bitlen_r) {
					*m_dst = (bits | (bits_n << bitlen)) & bits_mask_edge, m_dst++;
				}
				else {
					*m_dst = bits | (bits_n << bitlen);
					*(m_dst + 1) = (bits_n >> bitlen_r) & bits_mask_edge, m_dst += 2;
				}

				bitlen_r = bitlen_rem;
				bitlen = 32 - bitlen_rem;
				bits = bits_n >> bitlen_r;
			}
		}
	}
	int bitlen_rem = size_rem_32;
	if (!bitlen_rem) return std::make_pair(dest, dword_width);

	const unsigned char* p_srcc = (const unsigned char*)m_src;
	uint32_t bits_last = (uint32_t)(p_srcc[0]);
	if (8 * 1 < bitlen_rem) bits_last |= ((uint32_t)(p_srcc[1]) << (8 * 1));
	if (8 * 2 < bitlen_rem) bits_last |= ((uint32_t)(p_srcc[2]) << (8 * 2));
	if (8 * 3 < bitlen_rem) bits_last |= ((uint32_t)(p_srcc[3]) << (8 * 3));
	if (fmbits & BTCPR_FM_B1W0) bits_last = ~bits_last;

	if (!bitlen) *m_dst = bits_last & bits_mask_edge;
	else if (bitlen_rem <= bitlen_r) {
		*m_dst = (bits | (bits_last << bitlen)) & bits_mask_edge;
	}
	else {
		*m_dst = bits | (bits_last << bitlen);
		*(m_dst + 1) = (bits_last >> bitlen_r) & bits_mask_edge;
	}
	return std::make_pair(dest, dword_width);
}
inline std::pair<const uint32_t*, int> CCL_Modify_Padding_X86(const void* source, int height, int width, int data_width_src, int fmbits) {
	int width_div_32 = width / 32;
	int width_rem_32 = width % 32;
	int dword_width_dst = width_div_32 + 1;
	uint32_t bits_mask_edge = ~(0xFFFFFFFF << width_rem_32);

	uint32_t* dest = new uint32_t[height * dword_width_dst];
	uint32_t* m_dst = dest;
	const unsigned char* m_src_c = (const unsigned char*)source;
	const unsigned char* m_src_c_end = m_src_c + height * data_width_src;

	if (fmbits & BTCPR_FM_B1W0) {
		for (; m_src_c < m_src_c_end; m_src_c += data_width_src) {
			const uint32_t* m_src = (const uint32_t*)m_src_c;
			const uint32_t* m_dst_end = m_dst + width_div_32;
			for (; m_dst < m_dst_end; m_dst++, m_src++) *m_dst = ~*m_src;

			const unsigned char* m_src_c_e = (const unsigned char*)m_src;
			uint32_t last_val = m_src_c_e[0];
			if (width_rem_32 > 8 * 1) last_val |= (uint32_t)(m_src_c_e[1]) << (8 * 1);
			if (width_rem_32 > 8 * 2) last_val |= (uint32_t)(m_src_c_e[2]) << (8 * 2);
			if (width_rem_32 > 8 * 3) last_val |= (uint32_t)(m_src_c_e[3]) << (8 * 3);
			*m_dst = (~last_val) & bits_mask_edge, m_dst++;
		}
	}
	else {
		for (; m_src_c < m_src_c_end; m_src_c += data_width_src) {
			const uint32_t* m_src = (const uint32_t*)m_src_c;
			const uint32_t* m_dst_end = m_dst + width_div_32;
			for (; m_dst < m_dst_end; m_dst++, m_src++) *m_dst = *m_src;

			const unsigned char* m_src_c_e = (const unsigned char*)m_src;
			uint32_t last_val = m_src_c_e[0];
			if (width_rem_32 > 8 * 1) last_val |= (uint32_t)(m_src_c_e[1]) << (8 * 1);
			if (width_rem_32 > 8 * 2) last_val |= (uint32_t)(m_src_c_e[2]) << (8 * 2);
			if (width_rem_32 > 8 * 3) last_val |= (uint32_t)(m_src_c_e[3]) << (8 * 3);
			*m_dst = last_val & bits_mask_edge, m_dst++;
		}
	}
	return std::make_pair(dest, dword_width_dst);
}
inline std::pair<const uint32_t*, int> CCL_Format_Change_X86(const void* source, int height, int width, int data_width, int fmbits) {
	if (!data_width) {
		if (fmbits & BTCPR_FM_ALIGN_1) data_width = (width / 8) + ((width % 8) ? 1 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_2) data_width = (width / 16) * 2 + ((width % 16) ? 2 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_4) data_width = (width / 32) * 4 + ((width % 32) ? 4 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_8) data_width = (width / 64) * 8 + ((width % 64) ? 8 : 0);
		else if (width % 32) return CCL_Add_Padding_X86(source, height, width, fmbits);
		else data_width = width / 8;
	}

	if (!(width % 32)) {
		if (!(fmbits & BTCPR_FM_B1W0)) return std::make_pair((const uint32_t*)source, data_width / 4);
		return CCL_Bit_Invert_X86(source, height, width, data_width, fmbits);
	}
	if (data_width % sizeof(long)) return CCL_Modify_Padding_X86(source, height, width, data_width, fmbits);
	if (fmbits & BTCPR_FM_PADDING_ZERO) return std::make_pair((const uint32_t*)source, data_width / 4);
	return CCL_Clear_Padding_X86(source, height, width, data_width / sizeof(long), fmbits);
}

#endif