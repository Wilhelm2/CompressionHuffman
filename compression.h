#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 512

typedef struct s_data {
    unsigned char character;   // encoded character
    unsigned int occurrences;  // occurrences of the character in the file
    unsigned int encoding;     // number representing the compressed character
    unsigned int usedBits;     // used bits to encode
} data;

typedef struct s_Tree {
    data d;
    struct s_Tree* left;
    struct s_Tree* right;
} Tree;

typedef struct s_list {
    struct s_list* next;
    Tree* elt;
} list;

typedef struct s_decompressionMetadata {
    data* charactersEncoding;
    unsigned int nbCharacters;
    unsigned int totalCharacterToDecode;
} decompressionMetadata;

data* readCharacterOccurrences(char* filename);
list* transformArrayToSortedList(data* characters, unsigned int arraySize);
Tree* makeNewTreeElement(data element);

list* makeNewListElement(data characterInfo);
list* addSortedElement(list* elt, list* head);

Tree* buildTree(data* characters, unsigned int nbCharacters);
void printSortedList(list* sortedList);
Tree* concatTree(Tree* left, Tree* right);
void printTree(Tree* root);
void freeTree(Tree* t);

char* concat(const char* S1, const char* S2);
void writeHuffmannMetadata(unsigned int fd, data* encodedCharacters);
void writeTextToFile(unsigned int fd_write, unsigned int fd_read, data* encodedCharacters);
unsigned int countEncodedCharacters(data* encodedCharacters);
unsigned int countCharactersInFile(data* encodedCharacters);
void testWrite(unsigned int fd, void* data, unsigned int size);
int testRead(unsigned int fd, size_t size, void* buffer);

#endif