/*
 * Autor: David Durman (xdurma00)
 * Datum: 8.4.2008
 * Soubor: ahed.c
 * Komentar: Adaptivni Huffmanovo kodovani / dekodovani
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdarg.h>	//va_list, va_start, va_end
#include <assert.h>
#include "ahed.h"


// struktura uzlu stromu
typedef struct node t_node;
struct node {
  int freq;
  int character;
  int order;
  t_node* left;
  t_node* right;
  t_node* parent;
};

#define ZERO_NODE -1		// indikace ZERO uzlu
#define INNER_NODE -2		// indikace vnitrniho uzlu
#define MAX_NODES 513		// maximalni pocet uzlu ve strome
#define MAX_CODE_LENGTH 513	// maximalni delka kodu uzlu jako retezce


unsigned char outputBuffer= 0;	// buffer pro zapsani znaku do souboru
int outputBufferPos = 7;	// pozice v bufferu

unsigned char inputBuffer = 0;	// vstupni buffer
int inputBufferPos = -1;	// pozice v nem


/**
 * Tiskne chybu na stderr a ukonci program
 */
void AHEDError(const char *fmt, ...){
  va_list args;
  fprintf(stderr, "AHED ERROR: ");

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  //  exit(AHEDFail);
}

/**
 * Vrati dalsi bit v poradi ve vstupnim proudu
 * (pouzito pouze dekoderem)
 */
int AHEDGetInputBufferNextBit(FILE* file){
  int c;
  if (inputBufferPos == -1){	// musim ziskat dalsi byte
    c = getc(file);
    if (c == EOF)
      return EOF;

    inputBuffer = (unsigned char) c;
    inputBufferPos = 7;
  }

  int ret = (inputBuffer >> inputBufferPos) & 1;
  inputBufferPos--;

  return ret;
}

/**
 * Vrati cely byte ze vstupniho proudu, nebo EOF, pokud se narazilo na konec souboru
 * (pouzito pouze dekoderem)
 */
int AHEDGetInputBufferChar(FILE* file){
  int ret = 0;
  int i = 7;
  int next_bit;	// dalsi bit ze souboru (resp. z bufferu)

  for (; i >= 0; i--){
    next_bit = AHEDGetInputBufferNextBit(file);
    if (next_bit == 0)
      ret &= (~(1 << i));
    else if (next_bit == 1)
      ret |= (1 << i);
    else
      return EOF;
  }
  return ret;
}

/**
 * Prida bit urceny parametrem value do vystupniho bufferu
 * vraci 1, pokud byl pridan do souboru byte, jinak 0
 * (pouzito pouze koderem)
 */
int AHEDPutBit2Buffer(FILE* file, int value){
  if (value)
    outputBuffer |= (1 << outputBufferPos);
  else 
    outputBuffer &= (~(1 << outputBufferPos));

  outputBufferPos--;

  if (outputBufferPos == -1){
    putc(outputBuffer, file);
    outputBufferPos = 7;
    outputBuffer = 0;
    return 1;
  }
  return 0;
}

/**
 * Prida cely znak (byte) do vystupniho bufferu
 * vraci pocet pridanych bytu (tedy 1)
 * (pouzito pouze koderem)
 */
int AHEDPutChar2Buffer(FILE* file, unsigned char c){
  int i = 7;
  for (; i >= 0; i--)
    AHEDPutBit2Buffer(file, (c >> i) & 1);
  return 1;
}

/**
 * Vraci true, jestli se jiz znak c ve strome nachazi
 * (pouzito pouze koderem)
 */
bool AHEDFirstInput(t_node** ta, int c){
  int i;
  for (i = 0; i < MAX_NODES; i++){
    if (ta[i] == NULL)	// dosel jsem na konec pole
      return true;
    if (ta[i]->character == c)
      return false;
  };
  return true;
}

/**
 * Prida kod uzlu n do vystupniho bufferu
 * vraci pocet pridanych bytu
 * (pouzito pouze koderem)
 */
int AHEDOutputNodeCode(FILE* file, t_node* n){
  int writeBytes = 0;

  int i = 0;
  char code[MAX_CODE_LENGTH];

  t_node* tmp = n;
  while (tmp->parent != NULL){
    if (tmp->parent->left == tmp)
      code[i++] = '0';
    else if (tmp->parent->right == tmp)
      code[i++] = '1';
    tmp = tmp->parent;
  }

  // vystup kodu v obracenem poradi  
  while (--i >= 0)
    writeBytes += AHEDPutBit2Buffer(file, code[i] - '0');

  return writeBytes;
}

/**
 * Prida kod znaku c do vystupniho bufferu
 * vraci pocet pridanych bytu (tedy 1)
 * (Pouzito pouze koderem)
 */
int AHEDOutputCharCode(FILE* file, unsigned char c){
  return AHEDPutChar2Buffer(file, c);
}

/**
 * Aktualizuje strom
 * Zvedne frekvence rodicum popripade pretvori na Huffmanuv strom
 */
void AHEDActualizeTree(t_node** tree_array, t_node* actual_node){
  t_node* node = actual_node;
  t_node* sfho_node = NULL;  // uzel se stejnou frekvenci a vyssim poradim
  t_node* aux;	// pomocne ukazatele pro vymenu uzlu
  t_node* auxpar;
  t_node* node_parent_left;
  t_node* node_parent_right;

  int auxorder;	// pomocna pro swap poradi
  int i;

  // dokud node neni koren
  while (node->parent != NULL){

    // najdi uzel se stejnou frekvenci a vyssim poradim
    // (posledni takovy uzel, s vyjimkou rodice)
    sfho_node = NULL;
    i = node->order - 1;
    for (; i >= 0; i--){
      if (tree_array[i]->freq == node->freq
	  && tree_array[i] != node->parent)
	sfho_node = tree_array[i];
    }
    
    // uzel se stejnou frekvenci a vyssim poradim existuje
    if (sfho_node != NULL){
      // vymenim node a sfho_node

      // vymena podstromu
      assert(node != sfho_node);       
      
      node_parent_left = node->parent->left;
      node_parent_right = node->parent->right;

      if (sfho_node->parent->left == sfho_node)
	sfho_node->parent->left = node;
      else if (sfho_node->parent->right == sfho_node)
	sfho_node->parent->right = node;

      if (node_parent_left == node){
	node->parent->left = sfho_node;
      } else if (node_parent_right == node)
	node->parent->right = sfho_node;


      // vymena ukazatele na rodice
      auxpar = node->parent;
      node->parent = sfho_node->parent;
      sfho_node->parent = auxpar;

      // uprava v poli tree_array
      tree_array[sfho_node->order] = node;
      tree_array[node->order] = sfho_node;

      // uprava poradi v atributech uzlu
      auxorder = node->order;
      node->order = sfho_node->order;
      sfho_node->order = auxorder;
    }

    // zvyseni frekvence v uzlu node o 1
    node->freq++;

    // prechod na predka
    node = node->parent;
  }

  // zvyseni frekvence v koreni o 1
  node->freq++;
}

/*--------------------------------------------------.
 |  AHED main functions                              |
 `--------------------------------------------------*/

/* Nazev:
 *   AHEDEncoding
 * Cinnost:
 *   Funkce koduje vstupni soubor do vystupniho souboru a porizuje zaznam o kodovani.
 * Parametry:
 *   ahed - zaznam o kodovani
 *   inputFile - vstupni soubor (nekodovany)
 *   outputFile - vystupní soubor (kodovany)
 * Navratova hodnota: 
 *    0 - kodovani probehlo v poradku
 *    -1 - pri kodovani nastala chyba
 */
int AHEDEncoding(tAHED *ahed, FILE *inputFile, FILE *outputFile)
{
  if (ahed == NULL){
    AHEDError("record of coder / decoder was not allocated");
    return AHEDFail;
  }
  ahed->uncodedSize = 0;
  ahed->codedSize = 0;

  int i;
  t_node* tree_array[MAX_NODES];  // pole uzlu, kvuli poradi a lepsimu prochazeni stromu
  for (i = 0; i < MAX_NODES; i++)
    tree_array[i] = NULL;

  // ukazatel na koren stromu
  t_node* tree_root = NULL;
  
  // vytvoreni uzlu ZERO
  t_node* zero_node = malloc(sizeof(t_node));
  if (zero_node == NULL){
    AHEDError("not enough memory");
    return AHEDFail;
  }
  zero_node->freq = 0;
  zero_node->character = ZERO_NODE;
  zero_node->order = 0;	// pocatecni poradi
  zero_node->left = NULL;
  zero_node->right = NULL;
  zero_node->parent = NULL;

  tree_root = zero_node;	// ZERO je root
  tree_array[0] = zero_node;	

  int c;	// cteny symbol
  while ( (c = getc(inputFile)) != EOF ){
    ahed->uncodedSize++;

    if (AHEDFirstInput(tree_array, c)){
      /**
       * Znak c byl nacten poprve
       */

      ahed->codedSize += AHEDOutputNodeCode(outputFile, zero_node);
      ahed->codedSize += AHEDOutputCharCode(outputFile, (unsigned char) c);

      // uprava poradi ZERO -> posunul se ve strome dolu
      zero_node->order = zero_node->order + 2;

      // vytvoreni noveho uzlu se znakem c
      t_node* nodeX = malloc(sizeof(t_node));
      if (nodeX == NULL){
	AHEDError("not enough memory");
	return AHEDFail;
      }
      nodeX->freq = 1;
      nodeX->character = c;
      nodeX->order = zero_node->order - 1;
      nodeX->left = NULL;
      nodeX->right = NULL;

      // vytvoreni noveho vnitrniho uzlu
      t_node* nodeU = malloc(sizeof(t_node));
      if (nodeU == NULL){
	AHEDError("not enough memory");
	return AHEDFail;
      }
      nodeU->freq = 0;
      nodeU->character = INNER_NODE;
      nodeU->order = zero_node->order - 2; // nahradi ZERO
      nodeU->left = zero_node;
      nodeU->right = nodeX;
      nodeU->parent = zero_node->parent;   // rodic byvale postaveneho ZERO

      // Novy vnitrni uzel se napoji na leveho syna ZERO rodice
      // ZERO pluje smerem doleva dolu
      if (zero_node->parent != NULL)
	zero_node->parent->left = nodeU;

      nodeX->parent = nodeU;
      zero_node->parent = nodeU;

      // uprava pozic v poli
      tree_array[nodeU->order] = nodeU;
      tree_array[nodeX->order] = nodeX;
      tree_array[zero_node->order] = zero_node;

      AHEDActualizeTree(tree_array, nodeU);


    } else {
      /**
       * Znak c se jiz ve stromu nachazi
       */

      t_node* nodeX = NULL;
      // nalezeni uzlu se znakem c
      for (i = 0; i < MAX_NODES; i++){
	assert(tree_array[i] != NULL);	// uzel tam jiz je
	if (tree_array[i]->character == c){
	  nodeX = tree_array[i];
	  break;
	}
      }
      assert(nodeX != NULL);	// uzel tam jiz musi byt
      assert(nodeX->parent != NULL); // nemuze jit o koren
      ahed->codedSize += AHEDOutputNodeCode(outputFile, nodeX);
      AHEDActualizeTree(tree_array, nodeX);
    }
  }

  // konec souboru zarovnam nulami na byte
  while (outputBufferPos != 7){
    ahed->codedSize += AHEDPutBit2Buffer(outputFile, 0);
  }

  // uvolnim pamet stromu
  i = 0;
  for (; i < MAX_NODES; i++){
    if (tree_array[i] == NULL)
      break;
    free(tree_array[i]);
  }

  return AHEDOK;
}

/* Nazev:
 *   AHEDDecoding
 * Cinnost:
 *   Funkce dekoduje vstupni soubor do vystupniho souboru a porizuje zaznam o dekodovani.
 * Parametry:
 *   ahed - zaznam o dekodovani
 *   inputFile - vstupni soubor (kodovany)
 *   outputFile - vystupní soubor (nekodovany)
 * Navratova hodnota: 
 *    0 - dekodovani probehlo v poradku
 *    -1 - pri dekodovani nastala chyba
 */
int AHEDDecoding(tAHED *ahed, FILE *inputFile, FILE *outputFile)
{
  if (ahed == NULL){
    AHEDError("record of coder / decoder was not allocated");
    return AHEDFail;
  }
  ahed->uncodedSize = 0;
  ahed->codedSize = 0;

  int i;
  t_node* tree_array[MAX_NODES];	// pole uzlu, kvuli poradi
  for (i = 0; i < MAX_NODES; i++)
    tree_array[i] = NULL;

  t_node* tree_root = NULL;	// koren stromu

  // vytvoreni uzlu ZERO
  t_node* zero_node = malloc(sizeof(t_node));
  if (zero_node == NULL){
    AHEDError("not enough memory");
    return AHEDFail;
  }
  zero_node->freq = 0;
  zero_node->character = ZERO_NODE;
  zero_node->order = 0;	// pocatecni poradi
  zero_node->left = NULL;
  zero_node->right = NULL;
  zero_node->parent = NULL;

  tree_root = zero_node;	// ZERO je root
  tree_array[0] = zero_node;	

  bool not_enc_symbol = true;	// je symbol nekomprimovany?

  int c;	// cteny symbol
  int end = 0;	// indikator konce dekoderu
  while (!end){
    
    if (not_enc_symbol == true){
      /**
       * Symbol v nekomprimovane podobe
       */
      c = AHEDGetInputBufferChar(inputFile);
      if (c == EOF)	// konec souboru?
	break;
      ahed->codedSize++;

      putc(c, outputFile);	// vystup znaku (bytu)
      ahed->uncodedSize++;
      
      // uprava poradi ZERO -> posunul se ve strome dolu
      zero_node->order = zero_node->order + 2;

      // vytvoreni noveho uzlu se znakem c
      t_node* nodeX = malloc(sizeof(t_node));
      if (nodeX == NULL){
	AHEDError("not enough memory");
	return AHEDFail;
      }
      nodeX->freq = 1;
      nodeX->character = c;
      nodeX->order = zero_node->order - 1;
      nodeX->left = NULL;
      nodeX->right = NULL;

      // vytvoreni noveho vnitrniho uzlu
      t_node* nodeU = malloc(sizeof(t_node));
      if (nodeU == NULL){
	AHEDError("not enough memory");
	return AHEDFail;
      }
      nodeU->freq = 0;
      nodeU->character = INNER_NODE;
      nodeU->order = zero_node->order - 2; // nahradi ZERO
      nodeU->left = zero_node;
      nodeU->right = nodeX;
      nodeU->parent = zero_node->parent;   // rodic byvale postaveneho ZERO

      // Novy vnitrni uzel se napoji na leveho syna ZERO rodice
      // ZERO pluje smerem doleva dolu
      if (zero_node->parent != NULL)
	zero_node->parent->left = nodeU;

      nodeX->parent = nodeU;
      zero_node->parent = nodeU;

      // uprava pozic v poli
      tree_array[nodeU->order] = nodeU;
      tree_array[nodeX->order] = nodeX;
      tree_array[zero_node->order] = zero_node;

      AHEDActualizeTree(tree_array, nodeU);

      not_enc_symbol = false;

    } else {
      /**
       * Komprimovany symbol
       */

      t_node* p_node = tree_array[0];
      
      // kod musi vest k nejakemu listu

      int nextBit;
      // dokud nenarazim na list
      while (p_node->right != NULL && p_node->left != NULL){
	nextBit = AHEDGetInputBufferNextBit(inputFile);
	if (nextBit == 1)
	  p_node = p_node->right;
	else if (nextBit == 0)
	  p_node = p_node->left;
	else {	// dorazil jsem na konec souboru
	  end = true;
	  break;
	}

	// vstupni buffer naplnen -> zpracovan dalsi byte
	if (inputBufferPos == 6)
	  ahed->codedSize++;
      }
      // konec souboru?
      if (!end){
	// je nasledujici znak v nekomprimovane podobe?
	if (p_node->character == ZERO_NODE){
	  not_enc_symbol = true;
	} else {
	  // v p_node mam list se znakem
	  putc(p_node->character, outputFile);
	  ahed->uncodedSize++;
	  AHEDActualizeTree(tree_array, p_node);
	  not_enc_symbol = false;
	}
      }//endif (!end)
    }
  }//endwhile

  // uklid
  i = 0;
  for (; i < MAX_NODES; i++){
    if (tree_array[i] == NULL)
      break;
    free(tree_array[i]);
  }

  return AHEDOK;
}


