# Bit-Run Three Scan (BRTS)

Bit-Run Three Scan (BRTS) algorithm is not completely new. It's more like a special optimization of Run-Based Two-Scan (RBTS) algorithm found by Lifeng He, Yuyan Chao and Kenji Suzuki. It shares many basic idea of RBTS. This document doesn't deal with the RBTS algorithm in detail. If you want to know about their approach first, refer their official [paper](https://ieeexplore.ieee.org/document/4472694).

BRTS is consist of three steps. First, it finds every linear chunks 1 in every scan-lines and generate metadata about their starting position and ending position. These linear chunks will be simply called by *Run*, the word introduced by the three authors mentioned above. 

Second, it gives labels on those run metadata. For the first row, every runs get different labels. For each cut in rest rows, it checks if there are upper cuts which are connected to it first. If no upper cuts are connected to it, it gets new label. If only one upper cuts are connected to it, it gets the copy of the label. If many upper cuts are connected to it, those labels are merged and it gets the merged one. 

After the second step, one gets metadata on runs which saves their starting point, ending point and labels. Generating label map from these metadata is straightforward. This consist the last step. Each steps will be explained with code in the following section.



## First step: bit-scanning

The first step is to find starting position and ending position of every runs in each scanline. This can be done efficiently on 1-bit per pixel format using bit scan forward (BSF) instruction. See the following code. This code uses compiler-specific `_BitScanForward` function. In GCC, one may have to replace it with equivalent code using `__builtin_ffs`.

```
unsigned __int64 working_bits = *bits;
unsigned long basepos = 0, bitpos = 0;
for (;; runs++) {
	//find starting position
	while (!_BitScanForward64(&bitpos, working_bits)) {
		bits++, basepos += 64;
		if (bits == bit_final) {
			runs->start_pos = (short)0xFFFF;
			runs->end_pos = (short)0xFFFF;
			return runs + 1;
		}
		working_bits = *bits;
	}
	runs->start_pos = short(basepos + bitpos);
	
	//find ending position
	working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
	while (!_BitScanForward64(&bitpos, working_bits)) {
		bits++, basepos += 64;
		if (bits == bit_final) {
			runs->end_pos = short(basepos), runs++;
			runs->start_pos = (short)0xFFFF;
			runs->end_pos = (short)0xFFFF;
			return runs + 1;
		}
		working_bits = ~(*bits);
	}
	working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
	runs->end_pos = short(basepos + bitpos);
}
```

The code above look complex, but runs very fast. Surprisingly enough, even when the original data is 1-byte on pixel format, compressing it into 1-bit per pixel format and applying this trick turns out much faster than finding runs directly. (The compressing part don't take so long when compilers can optimize it with CMOV instruction.)



## Second step: label generating

The `Run` structure, which we have just used to save starting point and ending point of each runs, has one more field saving label of the run. The second step generates this field in CCL manner. See the code below.

```
Run* runs_up = runs;

//generate labels for the first row
for (; runs->start_pos != 0xFFFF; runs++) runs->label = labelsolver.NewLabel();
runs++;

//generate labels for the rests
for (; runs != runs_end; runs++) {
	Run* runs_save = runs;
	for (;; runs++) {
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
}
```

For the first row, every runs get different labels. For each cut in rest rows, it checks if there are upper cuts which are connected to it first. If no upper cuts are connected to it, it gets new label. If only one upper cuts are connected to it, it gets the copy of the label. If many upper cuts are connected to it, those labels are merged and it gets the merged one. 

Note that the first step and the second step is originally merged in RBTS algorithm. BRTS splits this step into two. It makes the algorithm slightly better in data locality aspect.

Also, the original RBTS algorithm writes labels directly into the output 2D label map. It write every pixels consist runs even if the run is very long and the label might turned out to be wrong and have to be modified later. I found that this is just waste of time. Saving the label on the run metadata makes the whole algorithm much faster.

RBTS way has its own advantages for it does not have to keep run metadata after they are checked. Using a circular array, the original RBTS effectively discards the metadata which don't need anymore. 

The total size of the run matadata may be big, but not absurdly so. it's just as big as the label map output itself even in the worst case. I think that this amount of temporal memory use would never be an issue in practice, especially in modern machine environment. Again, BRTS way turns out to be much faster than the original RBTS approach.



## Final step

The final step is straightforward. It just write the label map using the run metadata previously generated. The code is as follow.

```
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

