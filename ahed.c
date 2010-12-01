/*
 * Adaptive Huffman encoder/decoder.
 * @author: David Durman
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdarg.h>	//va_list, va_start, va_end
#include <assert.h>
#include "ahed.h"


// node structure
typedef struct node t_node;
struct node {
  int freq;
  int character;
  int order;
  t_node* left;
  t_node* right;
  t_node* parent;
};

#define ZERO_NODE -1		// ZERO node indicator
#define INNER_NODE -2		// inner node indicator
#define MAX_NODES 513		// maximum number of nodes in the tree
#define MAX_CODE_LENGTH 513	// maximum number of length of string code in a node


unsigned char outputBuffer= 0;	// byte buffer for output
int outputBufferPos = 7;	// position in byte buffer

unsigned char inputBuffer = 0;	// input buffer
int inputBufferPos = -1;	// input buffer position


/**
 * Print error to stderr and exit.
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
 * Get next bit from input stream.
 * (used only by decoder)
 */
int AHEDGetInputBufferNextBit(FILE* file){
  int c;
  if (inputBufferPos == -1){	// need to get another byte
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
 * Get byte from input stream or EOF.
 * (used only by decoder)
 */
int AHEDGetInputBufferChar(FILE* file){
  int ret = 0;
  int i = 7;
  int next_bit;	// next buffer bit

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
 * Put bit (value) to the output buffer.
 * @return 1 if whole byte was appended, 0 otherwise
 * (used only be encoder)
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
 * Put byte to the output buffer.
 * @return number of appended bytes (i.e. always 1).
 * (used only by encoder)
 */
int AHEDPutChar2Buffer(FILE* file, unsigned char c){
  int i = 7;
  for (; i >= 0; i--)
    AHEDPutBit2Buffer(file, (c >> i) & 1);
  return 1;
}

/**
 * @return true if c is in the tree, false otherwise
 * (used only by encoder)
 */
bool AHEDFirstInput(t_node** ta, int c){
  int i;
  for (i = 0; i < MAX_NODES; i++){
    if (ta[i] == NULL)	// reached end of the array
      return true;
    if (ta[i]->character == c)
      return false;
  };
  return true;
}

/**
 * Append node code n to the output buffer.
 * @return number of appended bytes
 * (used only by encoder)
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

  // output of the code is in reverse order
  while (--i >= 0)
    writeBytes += AHEDPutBit2Buffer(file, code[i] - '0');

  return writeBytes;
}

/**
 * Append code of character c to the output buffer.
 * @return number of appended bytes (i.e. 1)
 * (used only by encoder)
 */
int AHEDOutputCharCode(FILE* file, unsigned char c){
  return AHEDPutChar2Buffer(file, c);
}

/**
 * Update tree.
 * Increase frequency of parents or recreate Huffman tree if needed.
 */
void AHEDActualizeTree(t_node** tree_array, t_node* actual_node){
  t_node* node = actual_node;
  t_node* sfho_node = NULL;  // node with the same frequency and higher order
  t_node* aux;	// auxiliary pointer for nodes swap
  t_node* auxpar;
  t_node* node_parent_left;
  t_node* node_parent_right;

  int auxorder;	// auxiliary variable for order swap
  int i;

  // till node is not the root
  while (node->parent != NULL){

    // find the last node with the same frequency and higher order
    // (skip parent)
    sfho_node = NULL;
    i = node->order - 1;
    for (; i >= 0; i--){
      if (tree_array[i]->freq == node->freq
	  && tree_array[i] != node->parent)
	sfho_node = tree_array[i];
    }
    
    if (sfho_node != NULL){
      // swap node and sfho_node

      // swap subtrees
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


      // swap parent pointers
      auxpar = node->parent;
      node->parent = sfho_node->parent;
      sfho_node->parent = auxpar;

      //  update tree_array
      tree_array[sfho_node->order] = node;
      tree_array[node->order] = sfho_node;

      // update order
      auxorder = node->order;
      node->order = sfho_node->order;
      sfho_node->order = auxorder;
    }

    // higher frequency
    node->freq++;

    // go one level up
    node = node->parent;
  }

  // increase frequency in the root node
  node->freq++;
}

/** 
 * Encode the input file, save the result to the output file and log actions.
 * @param {tAHED} ahed encoding log
 * @param {FILE*} inputFile decoded input file
 * @param {FILE*} outputFile encoded output file
 * @return 0 if encoding went OK, -1 otherwise
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
  t_node* tree_array[MAX_NODES];  // nodes array, for easier order recognition and tree walking
  for (i = 0; i < MAX_NODES; i++)
    tree_array[i] = NULL;

  t_node* tree_root = NULL;
  
  // create ZERO node
  t_node* zero_node = malloc(sizeof(t_node));
  if (zero_node == NULL){
    AHEDError("not enough memory");
    return AHEDFail;
  }
  zero_node->freq = 0;
  zero_node->character = ZERO_NODE;
  zero_node->order = 0;
  zero_node->left = NULL;
  zero_node->right = NULL;
  zero_node->parent = NULL;

  tree_root = zero_node;	// ZERO is the root
  tree_array[0] = zero_node;	

  int c;	// read symbol
  while ( (c = getc(inputFile)) != EOF ){
    ahed->uncodedSize++;

    if (AHEDFirstInput(tree_array, c)){
      // Character c was seen for the first time.

      ahed->codedSize += AHEDOutputNodeCode(outputFile, zero_node);
      ahed->codedSize += AHEDOutputCharCode(outputFile, (unsigned char) c);

      // update ZERO -> move it down the tree
      zero_node->order = zero_node->order + 2;

      // create a new node with the character c
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

      // create new inner node
      t_node* nodeU = malloc(sizeof(t_node));
      if (nodeU == NULL){
	AHEDError("not enough memory");
	return AHEDFail;
      }
      nodeU->freq = 0;
      nodeU->character = INNER_NODE;
      nodeU->order = zero_node->order - 2; // will replace ZERO
      nodeU->left = zero_node;
      nodeU->right = nodeX;
      nodeU->parent = zero_node->parent;   // parent node of previous ZERO

      // connect the new node to the left child of ZERO parent
      // ZERO goes left down in the tree
      if (zero_node->parent != NULL)
	zero_node->parent->left = nodeU;

      nodeX->parent = nodeU;
      zero_node->parent = nodeU;

      // update tree array
      tree_array[nodeU->order] = nodeU;
      tree_array[nodeX->order] = nodeX;
      tree_array[zero_node->order] = zero_node;

      AHEDActualizeTree(tree_array, nodeU);


    } else {
      // Character c is already in the tree.

      t_node* nodeX = NULL;
      // find node with the character c
      for (i = 0; i < MAX_NODES; i++){
	assert(tree_array[i] != NULL);
	if (tree_array[i]->character == c){
	  nodeX = tree_array[i];
	  break;
	}
      }
      assert(nodeX != NULL);
      assert(nodeX->parent != NULL); // it cannot be the root node
      ahed->codedSize += AHEDOutputNodeCode(outputFile, nodeX);
      AHEDActualizeTree(tree_array, nodeX);
    }
  }

  // align to the byte with zeros
  while (outputBufferPos != 7){
    ahed->codedSize += AHEDPutBit2Buffer(outputFile, 0);
  }

  // free memory occupied by the tree
  i = 0;
  for (; i < MAX_NODES; i++){
    if (tree_array[i] == NULL)
      break;
    free(tree_array[i]);
  }

  return AHEDOK;
}

/** 
 * Decodes the input file, stores result to the output file and log actions.
 * @param {tAHED*} ahed decoding log
 * @param {FILE*} inputFile encoded input file
 * @param {FILE*} outputFile decoded output file
 * @return 0 decoding was OK, -1 otherwise
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
  t_node* tree_array[MAX_NODES];	
  for (i = 0; i < MAX_NODES; i++)
    tree_array[i] = NULL;

  t_node* tree_root = NULL;	

  // create ZERO node
  t_node* zero_node = malloc(sizeof(t_node));
  if (zero_node == NULL){
    AHEDError("not enough memory");
    return AHEDFail;
  }
  zero_node->freq = 0;
  zero_node->character = ZERO_NODE;
  zero_node->order = 0;
  zero_node->left = NULL;
  zero_node->right = NULL;
  zero_node->parent = NULL;

  tree_root = zero_node;	// ZERO is the root
  tree_array[0] = zero_node;	

  bool not_enc_symbol = true;	// is symbol uncompressed?

  int c;	// read symbol
  int end = 0;	// decoder end indicator
  while (!end){
    
    if (not_enc_symbol == true){
      // Symbol is uncompressed

      c = AHEDGetInputBufferChar(inputFile);
      if (c == EOF)
	break;
      ahed->codedSize++;

      putc(c, outputFile);	// output one byte
      ahed->uncodedSize++;
      
      // update ZERO node order -> move down the tree
      zero_node->order = zero_node->order + 2;

      // create a new node with character c
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

      // create a new inner node
      t_node* nodeU = malloc(sizeof(t_node));
      if (nodeU == NULL){
	AHEDError("not enough memory");
	return AHEDFail;
      }
      nodeU->freq = 0;
      nodeU->character = INNER_NODE;
      nodeU->order = zero_node->order - 2; // replaces ZERO
      nodeU->left = zero_node;
      nodeU->right = nodeX;
      nodeU->parent = zero_node->parent;   // parent of the previous ZERO node

      // connect the new inner node to the left child of the ZERO parent
      // ZERO goes to the left down in the tree
      if (zero_node->parent != NULL)
	zero_node->parent->left = nodeU;

      nodeX->parent = nodeU;
      zero_node->parent = nodeU;

      // update order in the tree array
      tree_array[nodeU->order] = nodeU;
      tree_array[nodeX->order] = nodeX;
      tree_array[zero_node->order] = zero_node;

      AHEDActualizeTree(tree_array, nodeU);

      not_enc_symbol = false;

    } else {
      // compressed symbol

      t_node* p_node = tree_array[0];
      
      // code has to end in a leaf node

      int nextBit;

      while (p_node->right != NULL && p_node->left != NULL){
	nextBit = AHEDGetInputBufferNextBit(inputFile);
	if (nextBit == 1)
	  p_node = p_node->right;
	else if (nextBit == 0)
	  p_node = p_node->left;
	else {	// end of file
	  end = true;
	  break;
	}

	// input buffer filled -> process the next byte
	if (inputBufferPos == 6)
	  ahed->codedSize++;
      }
      // end of file?
      if (!end){
	// is the next symbol uncompressed?
	if (p_node->character == ZERO_NODE){
	  not_enc_symbol = true;
	} else {
	  // p_node is a leaf node with the character
	  putc(p_node->character, outputFile);
	  ahed->uncodedSize++;
	  AHEDActualizeTree(tree_array, p_node);
	  not_enc_symbol = false;
	}
      }//endif (!end)
    }
  }//endwhile

  // cleanup
  i = 0;
  for (; i < MAX_NODES; i++){
    if (tree_array[i] == NULL)
      break;
    free(tree_array[i]);
  }

  return AHEDOK;
}


