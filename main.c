/*
 * Adaptive Huffman encoder/decoder.
 * @author: David Durman
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "ahed.h"


int main(int argc, char **argv)
{

  char *ifile = NULL;
  char *ofile = NULL;
  char *lfile = NULL;
  int lFlag = 0;
  int cFlag = 0;
  int xFlag = 0;
  int hFlag = 0;
  int c;

  FILE* inputFile = NULL;
  FILE* outputFile = NULL;
  FILE* logFile = NULL;

  opterr = 0;

  // process program arguments
  while ((c = getopt(argc, argv, "i:o:l:cxh")) != -1){
    switch (c){
    case 'i':
      ifile = optarg;
      break;
    case 'o':
      ofile = optarg;
      break;
    case 'l':
      lfile = optarg;
      lFlag = 1;
      break;
    case 'c':
      cFlag = 1;
      break;
    case 'x':
      xFlag = 1;
      break;
    case 'h':
      hFlag = 1;
      break;
    case '?':
    default:
      AHEDError("unknown argument");
      return AHEDFail;
      break;
    }
  }

  // help
  if (hFlag == 1){
    printf("USAGE: ahed -h | -c | -x [-i input_file] [-o output_file] [-l log_file] \n");
    return AHEDOK;
  }

  // input file
  if (ifile == NULL)
    inputFile = stdin;
  else
    inputFile = fopen(ifile, "rb");
  if (inputFile == NULL){
    AHEDError("can not find an input file");
    return AHEDFail;
  }

  // output file
  if (ofile == NULL)
    outputFile = stdout;
  else
    outputFile = fopen(ofile, "wb");
  if (outputFile == NULL){
    AHEDError("can not open an output file");
    return AHEDFail;
  }

  // log file
  if (lFlag == 1){
    if (lfile == NULL){
      AHEDError("can not open a log file");
      return AHEDFail;
    }
    else
      logFile = fopen(lfile, "w");
  }


  if (cFlag == 1 && xFlag == 1){
    AHEDError("please make a decision, encode or decode?\n");
    return AHEDFail;
  }

  // encoding/decoding log structure
  tAHED* ahed = malloc(sizeof(tAHED));
  if (ahed == NULL){
    AHEDError("not enough memory");
    return AHEDFail;
  }

  if (cFlag == 1)
    AHEDEncoding(ahed, inputFile, outputFile);
  else if (xFlag == 1)
    AHEDDecoding(ahed, inputFile, outputFile);  


  // log
  if (logFile != NULL){
    fprintf(logFile, "uncodedSize = %lu\n", ahed->uncodedSize);
    fprintf(logFile, "codedSize = %lu\n", ahed->codedSize);
    fclose(logFile);
  }

  // cleanup
  free(ahed);
  if (inputFile != NULL) fclose(inputFile);
  if (outputFile != NULL) fclose(outputFile);

  return AHEDOK;
}


	
