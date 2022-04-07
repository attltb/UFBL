#define _CRT_SECURE_NO_WARNINGS
#include "Formats.h"
#include "Format_changer_X86.h"
#include "Format_changer_X64.h"
#include "Labeling_BRTS_4c_x86.h"
#include "Labeling_BRTS_4c_x64.h"
#include "Labeling_BRTS_8c_x86.h"
#include "Labeling_BRTS_8c_x64.h"
#include "Labeling_BMRS_8c_x86.h"
#include "Labeling_BMRS_8c_x64.h"
#include "Labeling_RBTS_4c.h"
#include "Labeling_RBTS_8c.h"
#include "Labeling_TS_4c.h"
#include "Labeling_TS_8c.h"
#include "Labeling_Null.h"
#include "Tester.h"
#include "Printer.h"
#include <iostream>
using namespace std;
inline void Print_Performance(clock_t time, const char* algorithm_name) {
	printf("%s : %d\n", algorithm_name, time);
}
void Test_Performances(Byte_Source& src_byte, Bit_Source& src_bit, const char* data_name) {
	printf("Results (4-connectivity) for \"%s\" :\n", data_name);
	Print_Performance(Test_Performance(Labeling_Null, src_byte, 32), "null");
	Print_Performance(Test_Performance(Labeling_TS4, src_byte, 32), "two-pass");
	Print_Performance(Test_Performance(Labeling_RBTS4, src_byte, 32), "run-based");
	Print_Performance(Test_Performance(Labeling_BRTS4_X86, src_bit, 32), "bit-run (x86)");
	Print_Performance(Test_Performance(Labeling_BRTS4_X64, src_bit, 32), "bit-run (x64)");

	printf("\n");
	printf("Results (8-connectivity) for \"%s\" :\n", data_name);
	Print_Performance(Test_Performance(Labeling_TS8, src_byte, 32), "two-pass");
	Print_Performance(Test_Performance(Labeling_RBTS8, src_byte, 32), "run-based");
	Print_Performance(Test_Performance(Labeling_BRTS8_X86, src_bit, 32), "bit-run (x86)");
	Print_Performance(Test_Performance(Labeling_BRTS8_X64, src_bit, 32), "bit-run (x64)");
	Print_Performance(Test_Performance(Labeling_BMRS_X86, src_bit, 32), "bit-merge-run (x86)");
	Print_Performance(Test_Performance(Labeling_BMRS_X64, src_bit, 32), "bit-merge-run (x64)");
}
void Test_Output(Byte_Source& src_byte, Bit_Source& src_bit, const char* data_name) {
	char filename_out[512];
	strcpy(filename_out, data_name);
	char* pos_extension = strrchr(filename_out, '.');
	if (!pos_extension) pos_extension = strrchr(filename_out, 0);

	strcpy(pos_extension, "-tp4.bmp");
	PerformLabeling(Labeling_TS4, src_byte).export_as(filename_out);
	strcpy(pos_extension, "-rb4.bmp");
	PerformLabeling(Labeling_RBTS4, src_byte).export_as(filename_out);
	strcpy(pos_extension, "-bs4.bmp");
	PerformLabeling(Labeling_BRTS4_X64, src_bit).export_as(filename_out);
	strcpy(pos_extension, "-tp8.bmp");
	PerformLabeling(Labeling_TS8, src_byte).export_as(filename_out);
	strcpy(pos_extension, "-rb8.bmp");
	PerformLabeling(Labeling_RBTS8, src_byte).export_as(filename_out);
	strcpy(pos_extension, "-bs8.bmp");
	PerformLabeling(Labeling_BRTS8_X64, src_bit).export_as(filename_out);
	strcpy(pos_extension, "-bms.bmp");
	PerformLabeling(Labeling_BMRS_X64, src_bit).export_as(filename_out);
}

bool Test_Bitmap(const char* filename) {
	Byte_Source src_byte;
	Bit_Source src_bit;
	bool is_succeed = src_byte.Initialize_by_File(filename);
	if (!is_succeed) return false;
	src_bit.Initialize(src_byte, 0, BTCPR_FM_ALIGN_8 | BTCPR_FM_PADDING_ZERO);

	Test_Performances(src_byte, src_bit, filename);
	Test_Output(src_byte, src_bit, filename);

	printf("\n\n");
	return true;
}
bool Test_PBM4(const char* filename) {
	Byte_Source src_byte;
	Bit_Source src_bit;
	bool is_succeed = src_bit.Initialize_by_File(filename);
	if (!is_succeed) return false;
	src_byte.Initialize(src_bit);

	Test_Performances(src_byte, src_bit, filename);
	Test_Output(src_byte, src_bit, filename);

	printf("\n\n");
	return true;
}
void Test_Rand(int height, int width) {
	Byte_Source src_byte;
	Bit_Source src_bit;
	src_byte.Initialize_by_Rand(height, width);
	src_bit.Initialize(src_byte, 0, BTCPR_FM_ALIGN_8 | BTCPR_FM_PADDING_ZERO);

	Test_Performances(src_byte, src_bit, "random");
	printf("\n\n");
}

int main(int argc, char* argv[]) {
	//initializing
	srand((unsigned int)time(nullptr));

	//test input data or bitmap file
	for (int i = 1; i < argc; i++) {
		char* pos_extension = strrchr(argv[i], '.');
		if (strcmp(pos_extension, ".bmp") == 0) Test_Bitmap(argv[i]);
		else if (strcmp(pos_extension, ".pbm") == 0) Test_PBM4(argv[i]);
		else {
			printf("Test is only supported for 24-bit .bmp or .pbm p4 format.");
			int i = getchar();
			return 0;
		}
		printf("\n");
	}

	//test with randomly generated cases
	if (argc < 2) Test_Rand(480, 640);

	printf("Test end.");
	int i = getchar();
}
