#include "Formats.h"
#include <stdio.h>
#include <iostream>
using namespace std;
char GetLabel(unsigned label) {
	if (label < 10) return '0' + label;

	unsigned label_s = label - 10;
	if (label_s < 26) return 'a' + label_s;

	unsigned label_S = label_s - 26;
	if (label_S < 26) return 'A' + label_S;
	
	return '?';
}
void PrintLabel(const unsigned int* labels, int height, int width, bool space) {
	for (int i = 0; i < height; i++) {
		const unsigned int* labels_row = labels + i * width;
		for (int j = 0; j < width; j++) {
			if (space && !(j % 8)) printf(" ");
			printf("%c", GetLabel(labels_row[j]));
		}
		printf("\n");
	}
}

void PrintData(const unsigned int* source, int height, int width, bool space) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (space && !(x % 8)) cout << ' ';
			if (!!source[(width * y) + x]) cout << '1';
			else cout << '0';
		}
		cout << endl;
	}
	cout << endl;
}
void PrintCompressedBits_X86(uint32_t bits, bool space) {
	for (int i = 0; i < 32; i++) {
		if (space && !(i % 8)) cout << ' ';
		if (bits & ((uint32_t)1 << i)) cout << '1';
		else cout << '0';
	}
}
void PrintCompressedBits_X64(uint64_t bits, bool space) {
	for (int i = 0; i < 64; i++) {
		if (space && !(i % 8)) cout << ' ';
		if (bits & ((uint64_t)1 << i)) cout << '1';
		else cout << '0';
	}
}
void PrintCompressedBits_on_MsbFirst_X86(uint32_t bits, bool space) {
	for (int i = 0; i < 32; i++) {
		if (space && !(i % 8)) cout << ' ';
		if (bits & ((uint32_t)1 << (31 - i))) cout << '1';
		else cout << '0';
	}
}
void PrintCompressedBits_on_MsbFirst_X64(uint64_t bits, bool space) {
	for (int i = 0; i < 64; i++) {
		if (space && !(i % 8)) cout << ' ';
		if (bits & ((uint64_t)1 << (31 - i))) cout << '1';
		else cout << '0';
	}
}
void PrintCompressedBits(const void* bits, int height, int width, int data_width, int fmbits, int reverse, bool space) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int x_new = (reverse) ? reverse - x : x;
			int width_b = (reverse) ? reverse + 1 : width;

			if (space && !(x % 8)) cout << ' ';
			if (GetBit(bits, height, width_b, data_width, fmbits, y, x_new)) cout << '1';
			else cout << '0';
		}
		cout << endl;
	}
	cout << endl;
}