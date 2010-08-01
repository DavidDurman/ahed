/*
 * Autor: David Durman (xdurma00)
 * Datum: 8.4.2008
 * Soubor: main.c
 * Komentar: Adaptivni Huffmanovo kodovani / dekodovani
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

  // zpracovani argumentu programu
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

  // napoveda
  if (hFlag == 1){
    printf("USAGE: ahed -h | -c | -x [-i input_file] [-o output_file] [-l log_file] \n");
    return AHEDOK;
  }

  // vstupni soubor
  if (ifile == NULL)
    inputFile = stdin;
  else
    inputFile = fopen(ifile, "rb");
  if (inputFile == NULL){
    AHEDError("can not find an input file");
    return AHEDFail;
  }

  // vystupni soubor
  if (ofile == NULL)
    outputFile = stdout;
  else
    outputFile = fopen(ofile, "wb");
  if (outputFile == NULL){
    AHEDError("can not open an output file");
    return AHEDFail;
  }

  // logovaci soubor
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

  // zaznam o kodovani / dekodovani
  tAHED* ahed = malloc(sizeof(tAHED));
  if (ahed == NULL){
    AHEDError("not enough memory");
    return AHEDFail;
  }

  if (cFlag == 1)
    AHEDEncoding(ahed, inputFile, outputFile);
  else if (xFlag == 1)
    AHEDDecoding(ahed, inputFile, outputFile);  


  // vypis vystupni zpravy
  if (logFile != NULL){
    fprintf(logFile, "login = xdurma00\n");
    fprintf(logFile, "uncodedSize = %lu\n", ahed->uncodedSize);
    fprintf(logFile, "codedSize = %lu\n", ahed->codedSize);
    fclose(logFile);
  }

  free(ahed);

  // zavreni souboru
  if (inputFile != NULL) fclose(inputFile);
  if (outputFile != NULL) fclose(outputFile);

  return AHEDOK;
}


	
