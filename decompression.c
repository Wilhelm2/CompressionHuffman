#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "compression.h"

// Reads the metadata from file
decompressionMetadata readMetadata(unsigned int fd) {
    decompressionMetadata metaData;
    testRead(fd, sizeof(int), &metaData.nbCharacters);  // Reads the number of encoded characters
    printf("numbers of encoded characters %d\n", metaData.nbCharacters);
    metaData.charactersEncoding = calloc(metaData.nbCharacters, sizeof(data));
    for (unsigned int i = 0; i < metaData.nbCharacters; i++)  // Reads the metadata of each character
        testRead(fd, sizeof(data), &metaData.charactersEncoding[i]);
    return metaData;
}

// Reads the compressed text from file, decompresses it and saves it to output file
void HuffmannDecompressFile(Tree* tree, unsigned int fd_read) {
    unsigned int fd_write;
    unsigned int nbReadCharacters;
    char buffer_read[BUFFER_SIZE], buffer_write[BUFFER_SIZE];
    if ((fd_write = open("decompressed_file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        perror("open");
        exit(1);
    }

    unsigned int pos = 0;
    Tree* findCharTree = tree;  // Characters are decoded by going through the tree till finding the encoded character
    while ((nbReadCharacters = testRead(fd_read, BUFFER_SIZE, buffer_read)) != 0) {
        for (unsigned int i = 0; i < nbReadCharacters; i++) {
            for (unsigned int j = 0; j < 8; j++) {
                if (findCharTree->left == NULL && findCharTree->right == NULL) {  // character found
                    buffer_write[pos] = findCharTree->d.character;
                    pos++;
                    findCharTree = tree;       // Goes back to root to decode the next character
                    if (pos == BUFFER_SIZE) {  // Write buffer is full
                        testWrite(fd_write, buffer_write, BUFFER_SIZE);
                        pos = 0;
                        memset(buffer_write, 0, BUFFER_SIZE);
                    }
                }
                if (buffer_read[i] & (1 << j))
                    findCharTree = findCharTree->right;
                else
                    findCharTree = findCharTree->left;
            }
        }
    }

    if (findCharTree->left == NULL && findCharTree->right == NULL) {  // character found
        buffer_write[pos] = findCharTree->d.character;
    }
    testWrite(fd_write, buffer_write, pos);
}

int main(unsigned int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage %s: <filename>\n", argv[0]);
        exit(1);
    }
    unsigned int fd;

    if ((fd = open(argv[1], O_RDONLY, 0666)) == -1) {
        perror("open");
        exit(1);
    }

    decompressionMetadata metadata = readMetadata(fd);
    Tree* treeMetaData = buildTree(metadata.charactersEncoding, metadata.nbCharacters);
    printTree(treeMetaData);
    HuffmannDecompressFile(treeMetaData, fd);

    freeTree(treeMetaData);
    free(metadata.charactersEncoding);
    return 0;
}
