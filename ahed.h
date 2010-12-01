/*
 * Adaptive Huffman encoder/decoder.
 * @author: David Durman
 */ 

#ifndef __KKO_AHED_H__
#define __KKO_AHED_H__

#include <stdio.h>
#include <sys/types.h>

#define AHEDOK 0
#define AHEDFail -1

// Encoder/decoder log
typedef struct{
	/* size of the decoded string */
	int64_t uncodedSize;
	/* size of the encoded string */
	int64_t codedSize;
} tAHED;


/** 
 * Encodes the input file, stores the result to the output file and log actions.
 * @param {tAHED*} ahed encoding log
 * @param {FILE*} inputFile decoded input file
 * @param {FILE*} outputFile encoded output file
 * @return 0 if encoding went OK, -1 otherwise
 */
int AHEDEncoding(tAHED *ahed, FILE *inputFile, FILE *outputFile);
/** 
 * Decodes the input file, store result to the output file and log actions.
 * @param {tAHED*} ahed decoding log
 * @param {FILE*} inputFile encoded input file
 * @param {FILE*} outputFile decoded output file
 * @return 0 decoding was OK, -1 otherwise
 */
int AHEDDecoding(tAHED *ahed, FILE *inputFile, FILE *outputFile);

#endif
