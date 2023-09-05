#include "compression.h"

// Reads the occurrence of characters in filename
data* readCharacterOccurrences(char* filename) {
    unsigned char buffer[BUFFER_SIZE];
    unsigned int size = 0, fd;
    data* characterOccurrences = calloc(256, sizeof(data));
    for (unsigned int i = 0; i < 256; i++) {
        characterOccurrences[i].character = i;
        characterOccurrences[i].occurrences = 0;
    }

    if ((fd = open(filename, O_RDONLY, 0666)) == -1) {
        perror("open");
        exit(1);
    }

    while ((size = testRead(fd, BUFFER_SIZE, buffer)) > 0) {
        for (unsigned int i = 0; i < size; i++)
            characterOccurrences[buffer[i]].occurrences++;
    }
    close(fd);
    return characterOccurrences;
}

// Fills encodingTab by using the encoding of characters represented with tree
void fillEncodingIntoArray(Tree* tree, data* encodingTab, unsigned int encoding, unsigned char bit) {
    if (tree->left == NULL && tree->right == NULL) {
        //        printf("encodes %c as %d bits %d\n", tree->d.character, encoding, bit);
        encodingTab[tree->d.character].encoding = encoding;
        encodingTab[tree->d.character].usedBits = bit;
    } else {
        fillEncodingIntoArray(tree->left, encodingTab, encoding, bit + 1);
        fillEncodingIntoArray(tree->right, encodingTab, encoding + (1 << bit), bit + 1);
    }
}

// Prints the encoding of the characters occurring in the input file
void printEncoding(data* encodingTab) {
    for (unsigned int i = 0; i < 256; i++) {
        if (encodingTab[i].occurrences > 0) {
            printf("character %c is encoded as ", i);
            for (unsigned int j = encodingTab[i].usedBits; j > 0; j--) {
                if (encodingTab[i].encoding & (1 << (j - 1)))
                    printf("1");
                else
                    printf("0");
            }
            printf("\n");
        }
    }
}

// Compresses the file filename
void HuffmanCompressFile(char* filename, data* encodedCharacters) {
    unsigned int fd_write, fd_read;
    char* compressedFileName = concat(filename, "_compressed");

    if ((fd_read = open(filename, O_RDONLY, 0666)) == -1) {
        perror("open");
        exit(1);
    }

    if ((fd_write = open(compressedFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        perror("open");
        exit(1);
    }
    writeHuffmannMetadata(fd_write, encodedCharacters);
    writeTextToFile(fd_write, fd_read, encodedCharacters);

    free(compressedFileName);
    close(fd_write);
    close(fd_read);
}

// Writes the metadata to fd necessary to decode the file
void writeHuffmannMetadata(unsigned int fd, data* encodedCharacters) {
    unsigned int nbEncodedCharacters = countEncodedCharacters(encodedCharacters);
    unsigned int nbCharactersInFile = countCharactersInFile(encodedCharacters);

    testWrite(fd, &nbEncodedCharacters, sizeof(unsigned int));
    testWrite(fd, &nbCharactersInFile, sizeof(unsigned int));

    for (unsigned int i = 0; i < 256; i++) {
        if (encodedCharacters[i].occurrences > 0) {  // if the character occurs in input file
            testWrite(fd, &encodedCharacters[i], sizeof(data));
        }
    }
}

// Counts the number of encoded characters
unsigned int countEncodedCharacters(data* encodedCharacters) {
    unsigned int total = 0;
    for (unsigned int i = 0; i < 256; i++) {
        if (encodedCharacters[i].occurrences > 0)
            total++;
    }
    return total;
}

// Counts the number of characters in the file
unsigned int countCharactersInFile(data* encodedCharacters) {
    unsigned int total = 0;
    for (unsigned int i = 0; i < 256; i++) {
        if (encodedCharacters[i].occurrences > 0)
            total += encodedCharacters[i].occurrences;
    }
    return total;
}

// Reads the text from fd_read and encodes it into fd_write
void writeTextToFile(unsigned int fd_write, unsigned int fd_read, data* encodedCharacters) {
    unsigned char buffer_read[BUFFER_SIZE], buffer_write[BUFFER_SIZE];
    unsigned int nbReadCharacters;
    unsigned int posByte = 0, posBuffer = 0;

    memset(buffer_write, 0, BUFFER_SIZE);

    while ((nbReadCharacters = read(fd_read, buffer_read, BUFFER_SIZE)) != 0) {
        for (unsigned int i = 0; i < nbReadCharacters; i++) {  // for each read character
            for (unsigned int j = 0; j < encodedCharacters[buffer_read[i]].usedBits; j++) {
                // for each encoded bit of that character
                if (encodedCharacters[buffer_read[i]].encoding & (1 << j))  // if bit is set in encode
                    buffer_write[posBuffer] |= 1 << posByte;
                posByte++;
                if (posByte == 8)  // used all the bits of current byte
                {
                    posByte = 0;
                    posBuffer++;
                    if (posBuffer == BUFFER_SIZE)  // full buffer
                    {
                        testWrite(fd_write, buffer_write, BUFFER_SIZE);
                        posBuffer = 0;
                        memset(buffer_write, 0, BUFFER_SIZE);  // zeroes all bits
                    }
                }
            }
        }
    }
    testWrite(fd_write, buffer_write, posBuffer);
}

// Concatenates two strings
char* concat(const char* S1, const char* S2) {
    char* result = malloc(strlen(S1) + strlen(S2) + 1);
    strncpy(result, S1, strlen(S1));
    strncpy(result + strlen(S1), S2, strlen(S2));
    result[strlen(S1) + strlen(S2)] = '\0';
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage %s: <filename>\n", argv[0]);
        exit(1);
    }

    data* characters = readCharacterOccurrences(argv[1]);
    Tree* tree = buildTree(characters, 256);
    //    printTree(tree);
    fillEncodingIntoArray(tree, characters, 0, 0);
    //    printEncoding(characters);
    HuffmanCompressFile(argv[1], characters);

    free(characters);
    freeTree(tree);
    return 0;
}
