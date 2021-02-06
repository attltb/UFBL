# Bit-Run Two Scan (BRTS)

Bit-Run Two Scan (BRTS) algorithm is not completely new. It's more like a special optimization of Run-Based Two-Scan (RBTS) algorithm found by Lifeng He, Yuyan Chao and Kenji Suzuki. It shares many basic idea of RBTS. This document doesn't deal with the RBTS algorithm in detail. If you want to know about their approach first, refer their official [paper](https://ieeexplore.ieee.org/document/4472694).

BRTS is consist of two steps. First, it finds every linear chunks 1 in every scan-lines and generate metadata about their starting position and ending position and label. These linear chunks will be simply called by *Run*, the word introduced by the three authors mentioned above. 

For the first row, every runs get different labels. For each cut in rest rows, it checks if there are upper cuts which are connected to it first. If no upper cuts are connected to it, it gets new label. If only one upper cuts are connected to it, it gets the copy of the label. If many upper cuts are connected to it, those labels are merged and it gets the merged one. 

After the second step, one gets metadata on runs which saves their starting point, ending point and labels. Generating label map from these metadata is straightforward. This consist the last step. Each steps will be explained with code in the following section.



## First scan: bit-scanning

The run metadata generating consists of two steps logically separated. One is to find starting position and ending position of a run. This can be done efficiently on 1-bit per pixel format using bit scan forward (BSF) instruction. See the following code. This code uses compiler-specific `_BitScanForward` function. In GCC, one may have to replace it with equivalent code using `__builtin_ffs`.

```C++
const unsigned __int64* bits = bits_start + data_width * row;
const unsigned __int64* bit_final = bits + data_width;
unsigned __int64 working_bits = *bits;
unsigned long basepos = 0, bitpos = 0;
for (;; runs++) {
	//find starting position
	while (!_BitScanForward64(&bitpos, working_bits)) {
		bits++, basepos += 64;
		if (bits >= bit_final) {
			runs->start_pos = (short)0xFFFF;
			runs->end_pos = (short)0xFFFF;
			runs++;
			goto out;
		}
		working_bits = *bits;
	}
	runs->start_pos = short(basepos + bitpos);

	//find ending position
	working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
	while (!_BitScanForward64(&bitpos, working_bits)) {
		bits++, basepos += 64;
		if (bits == bit_final) {
			bitpos = 0;
			working_bits = 0;
			break;
		}
		working_bits = ~(*bits);
	}
	working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
	runs->end_pos = short(basepos + bitpos);

	... //[label generating]
}
out:
```

The code above look complex, but runs very fast. Surprisingly enough, even when the original data is 1-byte on pixel format, compressing it into 1-bit per pixel format and applying this trick turns out much faster than finding runs directly. (The compressing part don't take so long when compilers can optimize it with CMOV instruction.)



## First scan: label generating

The `Run` structure, which we have just used to save starting point and ending point of each runs, has one more field saving label of the run. After the starting point and the ending point is found, one has to generate this field in CCL manner. The full code is as below.

```C++
Run* runs_save = runs;
	
... //[initializing variables for bit-scanning]
for (;; runs++) {
	... //[bit-scanning]
   	
	unsigned short start_pos = runs->start_pos;
	if (start_pos == 0xFFFF) break;

	//Skip upper runs ends before this slice starts 
	for (; runs_up->end_pos < start_pos; runs_up++);

	//No upper run meets this
	unsigned short end_pos = runs->end_pos;
	if (runs_up->start_pos > end_pos) {
		runs->label = labelsolver.NewLabel();
		continue;
	};

	unsigned label = labelsolver.GetLabel(runs_up->label);

	//Next upper run can not meet this
	if (end_pos <= runs_up->end_pos) {
		runs->label = label;
		continue;
	}

	//Find next upper runs meet this
	runs_up++;
	for (; runs_up->start_pos <= end_pos; runs_up++) {
		unsigned label_other = labelsolver.GetLabel(runs_up->label);
		if (label != label_other) label = labelsolver.Merge(label, label_other);
		if (end_pos <= runs_up->end_pos) break;
	}
	runs->label = label;
}

runs_up = runs_save;
```

The variable `runs_up` points the run metadatas of the upper scanline, which is processed before working scanline. The first line should be treated differently since it does not have upper scanline. For the first row, every runs get different labels. The code above shows the run-generating code for the rest rows. 

The code checks if there are upper cuts which are connected to it first. If no upper cuts are connected to it, it gets new label. If only one upper cuts are connected to it, it gets the copy of the label. If many upper cuts are connected to it, those labels are merged and it gets the merged one. 

Note that unlike BRTS, the original RBTS algorithm writes labels directly into the 2D label map (the output). It writes every pixels consist a run even if the run is very long and the label might turned out to be wrong and have to be modified later. This is just waste of time. Saving the label on run metadata makes the whole algorithm much faster.

RBTS way has its own advantages for it does not have to keep metadatas which are already checked. Using a circular array, the original RBTS effectively discards the metadata which don't need anymore. 

The total size of the run matadata may be big, but not absurdly so. it's just almost as big as the label map output itself even in the worst case. I think that this amount of temporal memory use would never be an issue in practice, especially in modern machine environment. Again, BRTS way turns out to be much faster than the original RBTS approach.



## Second scan

The second scan is straightforward. It just write the label map using the run metadata previously generated. The code is as follow.

```C++
Run* runs = Data_run.runs;
for (size_t i = 0; i < height; i++) {
	unsigned* labels = dest + width * i;
	for (size_t j = 0;; runs++) {
		unsigned short start_pos = runs->start_pos;
		if (start_pos == 0xFFFF) {
			for (; j < width; j++) labels[j] = 0;
			runs++;
			break;
		}
		unsigned short end_pos = runs->end_pos;
		unsigned label = labelsolver.GetLabel(runs->label);
		for (; j < start_pos; j++) labels[j] = 0;
		for (; j < end_pos; j++) labels[j] = label;
	}
}
```

Since it calls `GetLabel` only once per a run, this final step also run slightly faster than usual second pass of the two-pass algorithm, including the one used in original RBTS. 

BRTS runs fast when the input data has relative simple structure, but inefficient when it's complex. See [Labeling_BMRS.md](Labeling_BMRS.md) for possible alternative. 

