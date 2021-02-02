#include "Common.h"
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <time.h>
#include <random>
extern void null_algorithm(const Data& data, Data& data_labels);
extern void bit_scanning_algorithm(const Data_Compressed& data, Data& data_labels);
extern void two_pass_algorithm(const Data& data, Data& data_labels);
extern void two_pass_algorithm_8c(const Data& data, Data& data_labels);
extern void run_based_algorithm_8c(const Data& data, Data& data_labels);
extern void bit_scanning_algorithm_8c(const Data_Compressed& data, Data& data_labels);
extern void bit_merge_scanning_algorithm(const Data_Compressed& data, Data& data_labels);

void InitCompressedData(const Data& data, Data_Compressed& data_compressed) {
	int height = data.height;
	int width = data.width;

	unsigned int* dest = data_compressed.bits;
	for (int i = 0; i < height; i++) {
		unsigned int* source = (unsigned int*)data.data[i];
		for (int j = 0; j < width >> 5; j++) {
			unsigned int obits = 0;
			if (source[0]) obits |= 0x00000001;
			if (source[1]) obits |= 0x00000002;
			if (source[2]) obits |= 0x00000004;
			if (source[3]) obits |= 0x00000008;
			if (source[4]) obits |= 0x00000010;
			if (source[5]) obits |= 0x00000020;
			if (source[6]) obits |= 0x00000040;
			if (source[7]) obits |= 0x00000080;
			if (source[8]) obits |= 0x00000100;
			if (source[9]) obits |= 0x00000200;
			if (source[10]) obits |= 0x00000400;
			if (source[11]) obits |= 0x00000800;
			if (source[12]) obits |= 0x00001000;
			if (source[13]) obits |= 0x00002000;
			if (source[14]) obits |= 0x00004000;
			if (source[15]) obits |= 0x00008000;
			if (source[16]) obits |= 0x00010000;
			if (source[17]) obits |= 0x00020000;
			if (source[18]) obits |= 0x00040000;
			if (source[19]) obits |= 0x00080000;
			if (source[20]) obits |= 0x00100000;
			if (source[21]) obits |= 0x00200000;
			if (source[22]) obits |= 0x00400000;
			if (source[23]) obits |= 0x00800000;
			if (source[24]) obits |= 0x01000000;
			if (source[25]) obits |= 0x02000000;
			if (source[26]) obits |= 0x04000000;
			if (source[27]) obits |= 0x08000000;
			if (source[28]) obits |= 0x10000000;
			if (source[29]) obits |= 0x20000000;
			if (source[30]) obits |= 0x40000000;
			if (source[31]) obits |= 0x80000000;
			*dest = obits, dest++, source += 32;
		}
		unsigned int obits = 0;
		for (int j = 0; j < width % 32; j++) {
			if (source[j]) obits |= (1 << j);
		}
		*dest = obits, dest++;
	}
}

Data* load_bmp24_as_data(const char* filename) {
	int fd = _open(filename, _O_BINARY | _O_RDONLY);
	if (fd == -1) {
		printf("파일 \"%s\"를 열지 못했습니다.\n", filename);
		return nullptr;
	}

	tagBITMAPFILEHEADER BitMapFileHeader;
	int count1 = _read(fd, &BitMapFileHeader, sizeof(tagBITMAPFILEHEADER));
	if (count1 != sizeof(tagBITMAPFILEHEADER) || BitMapFileHeader.bfType != 0x4D42) {
		printf("파일 \"%s\"이 올바른 24비트 비트맵 파일이 아닙니다.\n", filename);
		_close(fd);
		return nullptr;
	}

	tagBITMAPINFOHEADER BitMapInfoHeader;
	int count2 = _read(fd, &BitMapInfoHeader, sizeof(tagBITMAPINFOHEADER));
	if (count2 != sizeof(tagBITMAPINFOHEADER) ||
		BitMapInfoHeader.biBitCount != 24 ||
		BitMapInfoHeader.biCompression != BI_RGB ||
		BitMapInfoHeader.biClrUsed != 0) {
		printf("파일 \"%s\"이 올바른 24비트 비트맵 파일이 아닙니다.\n", filename);
		_close(fd);
		return nullptr;
	}

	int width = BitMapInfoHeader.biWidth;
	int height = BitMapInfoHeader.biHeight;
	int size_irrelevant = BitMapFileHeader.bfOffBits - sizeof(tagBITMAPFILEHEADER) - sizeof(tagBITMAPINFOHEADER);

	int data_length_row = width * sizeof(tagRGBTRIPLE);
	int row_padding = (4 - data_length_row % 4) % 4;
	int data_size = height * (data_length_row + row_padding);

	_lseek(fd, BitMapFileHeader.bfOffBits, SEEK_SET);
	BYTE* buffer_raw24 = new BYTE[data_size];
	if (_read(fd, buffer_raw24, data_size) != data_size) {
		printf("파일 \"%s\"이 올바른 24비트 비트맵 파일이 아닙니다.\n", filename);
		delete[] buffer_raw24;
		_close(fd);
		return nullptr;
	}
	_close(fd);

	Data* data = new Data(height, width);
	tagRGBQUAD* buffer_raw32 = (tagRGBQUAD*)data->raw;
	BYTE* rbuffer = buffer_raw24;
	tagRGBQUAD* wbuffer = buffer_raw32;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			wbuffer->rgbRed = ((tagRGBTRIPLE*)rbuffer)->rgbtRed;
			wbuffer->rgbGreen = ((tagRGBTRIPLE*)rbuffer)->rgbtGreen;
			wbuffer->rgbBlue = ((tagRGBTRIPLE*)rbuffer)->rgbtBlue;
			wbuffer->rgbReserved = 0;
			wbuffer++;
			rbuffer += 3;
		}
		rbuffer += row_padding;
	}

	delete[] buffer_raw24;
	return data;
}
bool save_as_bmp24(const Data& data, const char* filename) {
	int height = data.height;
	int width = data.width;

	int data_length_row = width * sizeof(tagRGBTRIPLE);
	int row_padding = (4 - data_length_row % 4) % 4;
	int data_size = height * (data_length_row + row_padding);
	BYTE* buffer_raw24 = new BYTE[data_size];

	tagRGBQUAD* rbuffer = (tagRGBQUAD*)data.raw;
	BYTE* wbuffer = buffer_raw24;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			((tagRGBTRIPLE*)wbuffer)->rgbtRed = rbuffer->rgbRed;
			((tagRGBTRIPLE*)wbuffer)->rgbtGreen = rbuffer->rgbGreen;
			((tagRGBTRIPLE*)wbuffer)->rgbtBlue = rbuffer->rgbBlue;
			rbuffer++;
			wbuffer += 3;
		}
		wbuffer += row_padding;
	}

	int fd_out = _open(filename, _O_BINARY | _O_WRONLY | _O_CREAT | O_TRUNC, _S_IREAD | _S_IWRITE);
	if (fd_out == -1) {
		printf("파일 %s를 생성할 수 없습니다.\n", filename);
		delete[] buffer_raw24;
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
	_close(fd_out);

	if (wcount != data_size * sizeof(BYTE)) {
		printf("파일 %s를 올바로 생성하지 못했습니다.\n", filename);
		return false;
	}
	return true;
}
Data& recolor_labels(Data& labels) {
	unsigned int color_rander = (unsigned int)rand();
	for (int i = 0; i < labels.height * labels.width; i++) {
		if (!labels.raw[i]) continue;
		unsigned int val = labels.raw[i] + color_rander;
		val = (val * 0x5D417C63) ^ (val * 0xC937EB2A);

		unsigned int r = val >> 24;
		unsigned int g = val >> 16;
		unsigned int b = val >> 8;
		r = 127 + ((r & 0x0F) << 3) | ((r & 0x70) >> 4);
		g = 127 + ((g & 0x0F) << 3) | ((g & 0x70) >> 4);
		b = 127 + ((b & 0x0F) << 3) | ((b & 0x70) >> 4);
		(*(tagRGBQUAD*)(&labels.raw[i])).rgbRed = (BYTE)r;
		(*(tagRGBQUAD*)(&labels.raw[i])).rgbGreen = (BYTE)g;
		(*(tagRGBQUAD*)(&labels.raw[i])).rgbBlue = (BYTE)b;
	}
	return labels;
};

unsigned int cal_clock_data(const Data& data, void (*algorithm) (const Data&, Data&)) {
	int height = data.height, width = data.width;
	Data* presults[32];
	for (int ix = 0; ix < 32; ix++) presults[ix] = new Data(height, width);

	clock_t clock_start = clock();
	for (int i = 0; i < 32; i++) algorithm(data, *(presults[i]));
	clock_t clock_end = clock();

	//memory release
	for (int i = 0; i < 32; i++) delete presults[i];
	return clock_end - clock_start;
}
unsigned int cal_clock_data(const Data& data, void (*algorithm) (const Data_Compressed&, Data&)) {
	int height = data.height, width = data.width;
	Data_Compressed data_compressed(height, width);
	InitCompressedData(data, data_compressed);

	Data* presults[32];
	for (int ix = 0; ix < 32; ix++) presults[ix] = new Data(height, width);

	clock_t clock_start = clock();
	for (int i = 0; i < 32; i++) algorithm(data_compressed, *(presults[i]));
	clock_t clock_end = clock();

	//memory release
	for (int i = 0; i < 32; i++) delete presults[i];
	return clock_end - clock_start;
}
unsigned int cal_clock_rand(int height, int width, void (*algorithm) (const Data&, Data&)) {
	Data* pdata = new Data(height, width);
	Data* presults[32];
	for (int ix = 0; ix < 32; ix++) presults[ix] = new Data(height, width);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			pdata->data[i][j] = rand() % 2;
		}
	}

	clock_t clock_start = clock();
	for (int i = 0; i < 32; i++) algorithm(*pdata, *(presults[i]));
	clock_t clock_end = clock();

	//memory release
	delete pdata;
	for (int i = 0; i < 32; i++) delete presults[i];
	return clock_end - clock_start;
}
unsigned int cal_clock_rand(int height, int width, void (*algorithm) (const Data_Compressed&, Data&)) {
	Data* pdata = new Data(height, width);
	Data* presults[32];
	for (int ix = 0; ix < 32; ix++) presults[ix] = new Data(height, width);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			pdata->data[i][j] = rand() % 2;
		}
	}

	Data_Compressed data_compressed(height, width);
	InitCompressedData(*pdata, data_compressed);

	clock_t clock_start = clock();
	for (int i = 0; i < 32; i++) algorithm(data_compressed, *(presults[i]));
	clock_t clock_end = clock();

	//memory release
	delete pdata;
	for (int i = 0; i < 32; i++) delete presults[i];
	return clock_end - clock_start;
}

bool test_bmp24(const char* filename) {
	Data* pdata = load_bmp24_as_data(filename);
	if (!pdata) {
		printf("Cannot open file %s.", filename);
		return false;
	}

	clock_t nul_t = cal_clock_data(*pdata, null_algorithm);
	clock_t tp_t = cal_clock_data(*pdata, two_pass_algorithm);
	clock_t bs_t = cal_clock_data(*pdata, bit_scanning_algorithm);

	printf("Results (4-connectivity) for \"%s\" :\n", filename);
	printf("null : %d\n", nul_t);
	printf("two-pass : %d\n", tp_t);
	printf("bit-run : %d\n", bs_t);

	printf("\n");
	clock_t tp8_t = cal_clock_data(*pdata, two_pass_algorithm_8c);
	clock_t rb_t = cal_clock_data(*pdata, run_based_algorithm_8c);
	clock_t bs8_t = cal_clock_data(*pdata, bit_scanning_algorithm_8c);
	clock_t bms_t = cal_clock_data(*pdata, bit_merge_scanning_algorithm);

	printf("Results (8-connectivity) for \"%s\" :\n", filename);
	printf("two-pass : %d\n", tp8_t);
	printf("run-based : %d\n", rb_t);
	printf("bit-run : %d\n", bs8_t);
	printf("bit-merge-run : %d\n", bms_t);

	//generate files as result
	Data_Compressed data_compressed(pdata->height, pdata->width);
	InitCompressedData(*pdata, data_compressed);

	Data* presult_tp = new Data(pdata->height, pdata->width);
	Data* presult_bs = new Data(pdata->height, pdata->width);
	two_pass_algorithm(*pdata, *presult_tp);
	bit_scanning_algorithm(data_compressed, *presult_bs);

	Data* presult_tp8 = new Data(pdata->height, pdata->width);
	Data* presult_rb = new Data(pdata->height, pdata->width);
	Data* presult_bs8 = new Data(pdata->height, pdata->width);
	Data* presult_bms = new Data(pdata->height, pdata->width);
	two_pass_algorithm_8c(*pdata, *presult_tp8);
	run_based_algorithm_8c(*pdata, *presult_rb);
	bit_scanning_algorithm_8c(data_compressed, *presult_bs8);
	bit_merge_scanning_algorithm(data_compressed, *presult_bms);

	char filename_out[512];
	strcpy(filename_out, filename);
	char* pos_extension = strrchr(filename_out, '.');
	if (!pos_extension) pos_extension = strrchr(filename_out, 0);

	strcpy(pos_extension, "-bs.bmp");
	save_as_bmp24(recolor_labels(*presult_bs), filename_out);
	strcpy(pos_extension, "-tp.bmp");
	save_as_bmp24(recolor_labels(*presult_tp), filename_out);
	strcpy(pos_extension, "-bs8.bmp");
	save_as_bmp24(recolor_labels(*presult_bs8), filename_out);
	strcpy(pos_extension, "-tp8.bmp");
	save_as_bmp24(recolor_labels(*presult_tp8), filename_out);
	strcpy(pos_extension, "-bms.bmp");
	save_as_bmp24(recolor_labels(*presult_bms), filename_out);
	strcpy(pos_extension, "-rb.bmp");
	save_as_bmp24(recolor_labels(*presult_rb), filename_out);

	delete pdata;
	delete presult_bs;
	delete presult_tp;
	delete presult_bs8;
	delete presult_tp8;
	delete presult_bms;
	delete presult_rb;
	return true;
}
void test_rand(int height, int width) {
	clock_t nul_t = cal_clock_rand(height, width, null_algorithm);
	clock_t tp_t = cal_clock_rand(height, width, two_pass_algorithm);
	clock_t bl_t = cal_clock_rand(height, width, bit_scanning_algorithm);

	printf("Results (4-connectivity) for random data :\n");
	printf("null : %d\n", nul_t);
	printf("two-pass : %d\n", tp_t);
	printf("bit-run : %d\n", bl_t);

	printf("\n");
	clock_t tp8_t = cal_clock_rand(height, width, two_pass_algorithm_8c);
	clock_t rb_t = cal_clock_rand(height, width, run_based_algorithm_8c);
	clock_t bl8_t = cal_clock_rand(height, width, bit_scanning_algorithm_8c);
	clock_t bms_t = cal_clock_rand(height, width, bit_merge_scanning_algorithm);

	printf("Results (8-connectivity) for random data :\n");
	printf("two-pass : %d\n", tp8_t);
	printf("run-based : %d\n", rb_t);
	printf("bit-run : %d\n", bl8_t);
	printf("bit-merge-run : %d\n", bms_t);
}

int main(int argc, char* argv[]) {
	//initializing
	srand((unsigned int)time(nullptr));

	//test input data or bitmap file
	for (int i = 1; i < argc; i++) {
		char* pos_extension = strrchr(argv[i], '.');
		if (strcmp(pos_extension, ".bmp") == 0) test_bmp24(argv[i]);
		else {
			printf("Test is only supported for 24-bit .bmp format.");
			int i = getchar();
			return 0;
		}
		printf("\n");
	}

	//test with randomly generated cases
	if (argc < 2) {
		test_rand(480, 640);
		printf("\n");
	}

	printf("Test end.");
	int i = getchar();
}