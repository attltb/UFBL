#define _CRT_SECURE_NO_WARNINGS
#include "Formats.h"
#include "Tester.h"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <sys/stat.h>
#include <windows.h>
#include <ctime>
bool LabelMap::is_equiv(LabelMap other) {
	if (height != other.height) return false;
	if (width != other.width) return false;

	int size = width * height;
	unsigned int label_max = 0;
	for (int i = 0; i < size; i++) {
		if (label_max < labels[i]) label_max = labels[i];
	}

	unsigned int* label_map = new unsigned int[label_max + 1];
	for (int i = 0; i < (int)label_max + 1; i++) label_map[i] = 0;
	for (int i = 0; i < size; i++) {
		unsigned int label_l = labels[i];
		unsigned int label_r = other.labels[i];
		if (label_l == label_r) continue;
		if (!label_l || !label_r) return false;
		if (!label_map[label_l]) label_map[label_l] = label_r;
		else if (label_map[label_l] != label_r) return false;
	}

	delete[] label_map;
	return true;
}
bool LabelMap::export_as(const char* filename) {
	//save labels as .bmp(24bit) file

	unsigned int color_rander = (unsigned int)rand();
	int size = height * width;
	tagRGBQUAD* buffer_raw32 = new tagRGBQUAD[size];
	for (int i = 0; i < height * width; i++) {
		tagRGBQUAD color{ (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0 };
		if (labels[i]) {
			unsigned int val = labels[i] + color_rander;
			val = (val * 0x5D417C63) ^ (val * 0xC937EB2A);

			unsigned int r = val >> 24;
			unsigned int g = val >> 16;
			unsigned int b = val >> 8;
			color.rgbRed = 127 + ((r & 0x0F) << 3) | ((r & 0x70) >> 4);
			color.rgbGreen = 127 + ((g & 0x0F) << 3) | ((g & 0x70) >> 4);
			color.rgbBlue = 127 + ((b & 0x0F) << 3) | ((b & 0x70) >> 4);
		}
		buffer_raw32[i] = color;
	}

	int data_length_row = width * sizeof(tagRGBTRIPLE);
	int row_padding = (4 - data_length_row % 4) % 4;
	int data_width = data_length_row + row_padding;
	int data_size = height * data_width;
	BYTE* buffer_raw24 = new BYTE[data_size];

	tagRGBQUAD* rbuffer = buffer_raw32;
	BYTE* wbuffer = buffer_raw24;
	for (int i = 0; i < height; i++) {
		tagRGBQUAD* rbuffer = buffer_raw32 + i * width;
		BYTE* wbuffer = buffer_raw24 + (height - 1 - i) * data_width;
		for (int j = 0; j < width; j++) {
			((tagRGBTRIPLE*)wbuffer)->rgbtRed = rbuffer->rgbRed;
			((tagRGBTRIPLE*)wbuffer)->rgbtGreen = rbuffer->rgbGreen;
			((tagRGBTRIPLE*)wbuffer)->rgbtBlue = rbuffer->rgbBlue;
			rbuffer++;
			wbuffer += 3;
		}
	}

	int fd_out = _open(filename, _O_BINARY | _O_WRONLY | _O_CREAT | O_TRUNC, _S_IREAD | _S_IWRITE);
	if (fd_out == -1) {
		printf("파일 %s를 생성할 수 없습니다.\n", filename);
		delete[] buffer_raw24;
		delete[] buffer_raw32;
		return false;
	}

	tagBITMAPFILEHEADER BitMapFileHeader;
	BitMapFileHeader.bfType = 'MB';
	BitMapFileHeader.bfSize = 54 + data_size * sizeof(BYTE);
	BitMapFileHeader.bfReserved1 = 0;
	BitMapFileHeader.bfReserved2 = 0;
	BitMapFileHeader.bfOffBits = 54;

	tagBITMAPINFOHEADER BitMapInfoHeader;
	BitMapInfoHeader.biSize = 40;
	BitMapInfoHeader.biWidth = width;
	BitMapInfoHeader.biHeight = height;
	BitMapInfoHeader.biPlanes = 1;
	BitMapInfoHeader.biBitCount = 24;
	BitMapInfoHeader.biCompression = BI_RGB;
	BitMapInfoHeader.biSizeImage = data_size * sizeof(BYTE);
	BitMapInfoHeader.biXPelsPerMeter = 0;
	BitMapInfoHeader.biYPelsPerMeter = 0;
	BitMapInfoHeader.biClrUsed = 0;
	BitMapInfoHeader.biClrImportant = 0;

	_write(fd_out, &BitMapFileHeader, sizeof(tagBITMAPFILEHEADER));
	_write(fd_out, &BitMapInfoHeader, sizeof(tagBITMAPINFOHEADER));
	int wcount = _write(fd_out, buffer_raw24, data_size * sizeof(BYTE));
	delete[] buffer_raw24;
	delete[] buffer_raw32;
	_close(fd_out);

	if (wcount != data_size * sizeof(BYTE)) {
		printf("파일 %s를 올바로 생성하지 못했습니다.\n", filename);
		return false;
	}
	return true;
}

void Byte_Source::Initialize(Bit_Source& bit_source) {
	height = bit_source.height;
	width = bit_source.width;
	data_width = width;
	data = new uint8_t[height * width];
	for (int y = 0; y < height; y++) {
		uint8_t* m_dst = data + y * width;
		for (int x = 0; x < width; x++) {
			m_dst[x] = GetBit(bit_source.data, height, width, bit_source.data_width, bit_source.fmbits, y, x);
		}
	}
}
void Byte_Source::Initialize_by_Rand(int _height, int _width) {
	height = _height;
	width = _width;
	data_width = width;
	data = new uint8_t[height * width];
	for (int i = 0; i < height; i++) {
		uint8_t* prow = data + i * width;
		for (int j = 0; j < width; j++) prow[j] = rand() % 2;
	}
}
bool Byte_Source::Initialize_by_File(const char* filename) {
	//load .bmp(24bit) file as source

	int fd = _open(filename, _O_BINARY | _O_RDONLY);
	if (fd == -1) {
		printf("파일 \"%s\"를 열지 못했습니다.\n", filename);
		return false;
	}

	tagBITMAPFILEHEADER BitMapFileHeader;
	int count1 = _read(fd, &BitMapFileHeader, sizeof(tagBITMAPFILEHEADER));
	if (count1 != sizeof(tagBITMAPFILEHEADER) || BitMapFileHeader.bfType != 0x4D42) {
		printf("파일 \"%s\"이 올바른 24비트 비트맵 파일이 아닙니다.\n", filename);
		_close(fd);
		return false;
	}

	tagBITMAPINFOHEADER BitMapInfoHeader;
	int count2 = _read(fd, &BitMapInfoHeader, sizeof(tagBITMAPINFOHEADER));
	if (count2 != sizeof(tagBITMAPINFOHEADER) ||
		BitMapInfoHeader.biBitCount != 24 ||
		BitMapInfoHeader.biCompression != BI_RGB ||
		BitMapInfoHeader.biClrUsed != 0) {
		printf("파일 \"%s\"이 올바른 24비트 비트맵 파일이 아닙니다.\n", filename);
		_close(fd);
		return false;
	}

	width = BitMapInfoHeader.biWidth;
	height = BitMapInfoHeader.biHeight;
	data_width = width;
	int size_irrelevant = BitMapFileHeader.bfOffBits - sizeof(tagBITMAPFILEHEADER) - sizeof(tagBITMAPINFOHEADER);

	int data_length_row = width * sizeof(tagRGBTRIPLE);
	int row_padding = (4 - data_length_row % 4) % 4;
	int row_byte_width = data_length_row + row_padding;
	int data_size = height * row_byte_width;

	_lseek(fd, BitMapFileHeader.bfOffBits, SEEK_SET);
	BYTE* buffer_raw24 = new BYTE[data_size];
	if (_read(fd, buffer_raw24, data_size) != data_size) {
		printf("파일 \"%s\"이 올바른 24비트 비트맵 파일이 아닙니다.\n", filename);
		delete[] buffer_raw24;
		_close(fd);
		return false;
	}
	_close(fd);

	data = new uint8_t[height * width];
	for (int i = 0; i < height; i++) {
		BYTE* rbuffer = buffer_raw24 + (height - 1 - i) * row_byte_width;
		uint8_t* wbuffer = data + i * width;
		for (int j = 0; j < width; j++) {
			tagRGBTRIPLE color = *((tagRGBTRIPLE*)rbuffer);
			if (color.rgbtRed || color.rgbtGreen || color.rgbtBlue) *wbuffer = 1;
			else *wbuffer = 0;
			wbuffer++;
			rbuffer += 3;
		}
	}

	delete[] buffer_raw24;
	return true;
}

inline bool str_skipline(const char* str, int& i, int size) {
	for (;; i++) {
		if (i >= size) return false;
		if (str[i] == '\n') {
			i++;
			break;
		}
	}
	return true;
}
inline int str_getint(const char* str, int& i, int size) {
	int value = atoi(str + i);
	for (;; i++) {
		if (i >= size) return -1;
		if (isspace(str[i])) {
			i++;
			break;
		}
	}
	return value;
}
void Bit_Source::Initialize(Byte_Source& byte_source, int _data_width, int _fmbits) {
	height = byte_source.height;
	width = byte_source.width;
	data_width = _data_width;
	fmbits = _fmbits;
	if (height == 0 || width == 0) return;
	if (!data_width) {
		if (fmbits & BTCPR_FM_ALIGN_1) data_width = (width / 8) + ((width % 8) ? 1 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_2) data_width = (width / 16) * 2 + ((width % 16) ? 2 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_4) data_width = (width / 32) * 4 + ((width % 32) ? 4 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_8) data_width = (width / 64) * 8 + ((width % 64) ? 8 : 0);
		else {
			int size = width * height;
			int size_byte_full = size / 8;
			int size_rem = size % 8;
			int size_byte = size_byte_full + (size_rem != 0);
			unsigned char* data_compressed = new unsigned char[size_byte];
			data = data_compressed;

			const uint8_t* m_src = byte_source.data;
			if (fmbits & BTCPR_FM_MSB_FIRST) {
				for (int i = 0; i < size_byte_full; i++, m_src += 8) {
					unsigned char obits = 0;
					if (m_src[0]) obits |= 0x80;
					if (m_src[1]) obits |= 0x40;
					if (m_src[2]) obits |= 0x20;
					if (m_src[3]) obits |= 0x10;
					if (m_src[4]) obits |= 0x08;
					if (m_src[5]) obits |= 0x04;
					if (m_src[6]) obits |= 0x02;
					if (m_src[7]) obits |= 0x01;
					if (fmbits & BTCPR_FM_B1W0) obits = ~obits;
					data_compressed[i] = obits;
				}
			}
			else {
				for (int i = 0; i < size_byte_full; i++, m_src += 8) {
					unsigned char obits = 0;
					if (m_src[0]) obits |= 0x01;
					if (m_src[1]) obits |= 0x02;
					if (m_src[2]) obits |= 0x04;
					if (m_src[3]) obits |= 0x08;
					if (m_src[4]) obits |= 0x10;
					if (m_src[5]) obits |= 0x20;
					if (m_src[6]) obits |= 0x40;
					if (m_src[7]) obits |= 0x80;
					if (fmbits & BTCPR_FM_B1W0) obits = ~obits;
					data_compressed[i] = obits;
				}
			}

			if (size_rem) {
				unsigned char obits = 0;
				if (fmbits & BTCPR_FM_MSB_FIRST) {
					if (fmbits & BTCPR_FM_B1W0) {
						for (int j = 0; j < size_rem; j++) {
							if (!m_src[j]) obits |= (0x80 >> j);
						}
					}
					else {
						for (int j = 0; j < size_rem; j++) {
							if (m_src[j]) obits |= (0x80 >> j);
						}
					}
					if (fmbits & BTCPR_FM_PADDING_MSB) obits >>= (8 - size_rem);
				}
				else {
					if (fmbits & BTCPR_FM_B1W0) {
						for (int j = 0; j < size_rem; j++) {
							if (!m_src[j]) obits |= (0x01 << j);
						}
					}
					else {
						for (int j = 0; j < size_rem; j++) {
							if (m_src[j]) obits |= (0x01 << j);
						}
					}
				}
				data_compressed[size_byte_full] = obits;
			}
			return;
		}
	}

	int width_byte_full = width / 8;
	int width_rem = width % 8;
	unsigned char* data_compressed = new unsigned char[height * data_width];
	unsigned char bits_mask_edge = (unsigned char)~(0xFF << (width % 8));
	data = data_compressed;

	for (int i = 0; i < height; i++) {
		unsigned char* m_dst = data_compressed + i * data_width;
		const uint8_t* m_src = byte_source.data + i * width;

		if (fmbits & BTCPR_FM_MSB_FIRST) {
			for (int j = 0; j < width_byte_full; j++, m_src += 8) {
				unsigned char obits = 0;
				if (m_src[0]) obits |= 0x80;
				if (m_src[1]) obits |= 0x40;
				if (m_src[2]) obits |= 0x20;
				if (m_src[3]) obits |= 0x10;
				if (m_src[4]) obits |= 0x08;
				if (m_src[5]) obits |= 0x04;
				if (m_src[6]) obits |= 0x02;
				if (m_src[7]) obits |= 0x01;
				if (fmbits & BTCPR_FM_B1W0) obits = ~obits;
				m_dst[j] = obits;
			}
		}
		else {
			for (int j = 0; j < width_byte_full; j++, m_src += 8) {
				unsigned char obits = 0;
				if (m_src[0]) obits |= 0x01;
				if (m_src[1]) obits |= 0x02;
				if (m_src[2]) obits |= 0x04;
				if (m_src[3]) obits |= 0x08;
				if (m_src[4]) obits |= 0x10;
				if (m_src[5]) obits |= 0x20;
				if (m_src[6]) obits |= 0x40;
				if (m_src[7]) obits |= 0x80;
				if (fmbits & BTCPR_FM_B1W0) obits = ~obits;
				m_dst[j] = obits;
			}
		}

		if (width_rem) {
			unsigned char obits = 0;
			if (fmbits & BTCPR_FM_MSB_FIRST) {
				if (fmbits & BTCPR_FM_B1W0) {
					for (int j = 0; j < width_rem; j++) {
						if (!m_src[j]) obits |= (0x80 >> j);
					}
				}
				else {
					for (int j = 0; j < width_rem; j++) {
						if (m_src[j]) obits |= (0x80 >> j);
					}
				}
				if (fmbits & BTCPR_FM_PADDING_MSB) obits >>= (8 - width_rem);
			}
			else {
				if (fmbits & BTCPR_FM_B1W0) {
					for (int j = 0; j < width_rem; j++) {
						if (!m_src[j]) obits |= (0x01 << j);
					}
				}
				else {
					for (int j = 0; j < width_rem; j++) {
						if (m_src[j]) obits |= (0x01 << j);
					}
				}
			}

			m_dst[width_byte_full] = obits;
		}

		int fullpadding_start = (width_rem) ? width_byte_full + 1 : width_byte_full;
		if (fmbits & BTCPR_FM_PADDING_ZERO) {
			for (int i = fullpadding_start; i < data_width; i++) m_dst[i] = 0;
		}
	}
}
void Bit_Source::Initialize_by_Rand(int _height, int _width, int _data_width, int _fmbits) {
	height = _height;
	width = _width;
	data_width = _data_width;
	fmbits = _fmbits;
	if (height == 0 || width == 0) return;
	if (!data_width) {
		if (fmbits & BTCPR_FM_ALIGN_1) data_width = (width / 8) + ((width % 8) ? 1 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_2) data_width = (width / 16) * 2 + ((width % 16) ? 2 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_4) data_width = (width / 32) * 4 + ((width % 32) ? 4 : 0);
		else if (fmbits & BTCPR_FM_ALIGN_8) data_width = (width / 64) * 8 + ((width % 64) ? 8 : 0);
		else {
			int size = width * height;
			int size_byte_full = size / 8;
			int size_rem = size % 8;
			int size_byte = size_byte_full + (size_rem != 0);
			unsigned char* data_compressed = new unsigned char[size_byte];
			data = data_compressed;

			int size_int = size_byte / sizeof(int);
			int size_int_rem = size_byte % sizeof(int);
			int i_rem_start = size_int * sizeof(int);

			for (int i = 0; i < size_int; i++) ((int*)data_compressed)[i] = rand();
			if (size_int_rem) {
				int rand_l = rand();
				for (int i = i_rem_start; i < size_int_rem; i++) data_compressed[i] = (unsigned char)(rand_l << 8 * i);
			}
			if (size_rem) {
				unsigned char mask = (unsigned char)~(0xFFFFFFFF << size_rem);
				data_compressed[size_byte_full] &= mask;
			}	
			return;
		}
	}

	int width_byte_full = width / 8;
	int width_rem = width % 8;

	int width_int = data_width / sizeof(int);
	int width_int_rem = data_width % sizeof(int);
	int i_rem_start = width_int * sizeof(int);

	unsigned char* data_compressed = new unsigned char[height * data_width];
	unsigned char bits_mask_edge = (unsigned char)~(0xFF << (width % 8));
	data = data_compressed;

	for (int i = 0; i < height; i++) {
		unsigned char* m_dst = data_compressed + i * data_width;
		for (int i = 0; i < width_int; i++) ((int*)m_dst)[i] = rand();
		if (width_int_rem) {
			int rand_l = rand();
			for (int i = i_rem_start; i < width_int_rem; i++) m_dst[i] = (unsigned char)(rand_l << 8 * i);
		}
		if (width_rem) {
			unsigned char mask = (unsigned char)~(0xFFFFFFFF << width_rem);
			m_dst[width_byte_full] &= mask;
		}

		int fullpadding_start = (width_rem) ? width_byte_full + 1 : width_byte_full;
		if (fmbits & BTCPR_FM_PADDING_ZERO) {
			for (int i = fullpadding_start; i < data_width; i++) m_dst[i] = 0;
		}
	}
}
bool Bit_Source::Initialize_by_File(const char* filename) {
	//load .pbm(p4) file as source

	int fd = _open(filename, _O_BINARY | _O_RDONLY);
	if (fd == -1) {
		printf("파일 \"%s\"를 열지 못했습니다.\n", filename);
		return false;
	}

	short P4Header;
	int count1 = _read(fd, &P4Header, sizeof(short));
	if (count1 != sizeof(P4Header) || P4Header != 0x3450) {
		printf("파일 \"%s\"이 pbm(p4) 파일이 아닙니다.\n", filename);
		_close(fd);
		return false;
	}

	_lseek(fd, 0, SEEK_END);
	int datasize = _tell(fd) - sizeof(short);
	char* filedata = new char[datasize];

	_lseek(fd, sizeof(short), SEEK_SET);
	if (_read(fd, filedata, datasize) != datasize) {
		printf("파일 \"%s\"이 올바른 pbm(p4) 파일이 아닙니다..\n", filename);
		delete[] filedata;
		_close(fd);
		return false;
	}
	_close(fd);

	try {
		int i = 0;
		if (filedata[i] == '\n') i++;
		if (i >= datasize) throw 1;
		for (; filedata[i] == '#';) {
			if (!str_skipline(filedata, i, datasize)) throw 1;
		}

		fmbits = BTCPR_FM_PBM4;
		width = str_getint(filedata, i, datasize);
		height = str_getint(filedata, i, datasize);
		if (width == -1 || height == -1) throw 2;
		if (!width || !height) {
			data = nullptr;
			return true;
		}

		int data_width = width / 8 + (width % 8 != 0);
		int size = height * data_width;
		data = new char[size];
		if (datasize - i < size) throw 3;
		memcpy(data, filedata + i, size);
	}
	catch (...) {
		printf("파일 \"%s\"이 올바른 pbm(p4) 파일이 아닙니다..\n", filename);
		delete[] filedata;
		_close(fd);
		return false;
	}

	return true;
}

LabelMap PerformLabeling(Byte_Algorithm byte_algorithm, Byte_Source& byte_source) {
	unsigned int* labels = new unsigned int[byte_source.height * byte_source.width];
	byte_algorithm(labels, byte_source.data, byte_source.height, byte_source.width, byte_source.data_width, 0);
	return LabelMap(labels, byte_source.height, byte_source.width);
}
LabelMap PerformLabeling(Bit_Algorithm bit_algorithm, Bit_Source& bit_source) {
	unsigned int* labels = new unsigned int[bit_source.height * bit_source.width];
	bit_algorithm(labels, bit_source.data, bit_source.height, bit_source.width, bit_source.data_width, bit_source.fmbits);
	return LabelMap(labels, bit_source.height, bit_source.width);
}

bool Test_Correctness(Byte_Algorithm byte_algorithm, Byte_Source& byte_source, Bit_Algorithm bit_algoritm, Bit_Source& bit_source) {
	LabelMap result_1 = PerformLabeling(byte_algorithm, byte_source);
	LabelMap result_2 = PerformLabeling(bit_algoritm, bit_source);
	return result_1.is_equiv(result_2);
}
bool Test_Correctness(Byte_Algorithm byte_algorithm, Byte_Source& byte_source, Byte_Algorithm byte_algoritm_t, Byte_Source& byte_source_t) {
	LabelMap result_1 = PerformLabeling(byte_algorithm, byte_source);
	LabelMap result_2 = PerformLabeling(byte_algoritm_t, byte_source_t);
	return result_1.is_equiv(result_2);
}
int Test_Performance(Byte_Algorithm byte_algorithm, Byte_Source& byte_source, int count) {
	int label_count = byte_source.height * byte_source.width;
	unsigned int** label_result = new unsigned int* [count];
	for (int i = 0; i < count; i++) label_result[i] = new unsigned int[label_count];

	clock_t clock_st = clock();
	for (int i = 0; i < count; i++) byte_algorithm(label_result[i], byte_source.data, byte_source.height, byte_source.width, byte_source.data_width, 0);
	clock_t clock_ed = clock();

	for (int i = 0; i < count; i++) delete[] label_result[i];
	delete[] label_result;
	return clock_ed - clock_st;
}
int Test_Performance(Bit_Algorithm bit_algorithm, Bit_Source& bit_source, int count) {
	int label_count = bit_source.height * bit_source.width;
	unsigned int** label_result = new unsigned int* [count];
	for (int i = 0; i < count; i++) label_result[i] = new unsigned int[label_count];

	clock_t clock_st = clock();
	for (int i = 0; i < count; i++) bit_algorithm(label_result[i], bit_source.data, bit_source.height, bit_source.width, bit_source.data_width, bit_source.fmbits);
	clock_t clock_ed = clock();

	for (int i = 0; i < count; i++) delete[] label_result[i];
	delete[] label_result;
	return clock_ed - clock_st;
}