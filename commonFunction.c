// File containing the function used by both the compression and decompression

#include "compression.h"

// Returns a sorted list built from the array characters of size arraySize
list* buildSortedListFromArray(data* characters, unsigned int arraySize) {
    list* sortedList = NULL;
    for (unsigned int i = 0; i < arraySize; i++) {
        if (characters[i].occurrences > 0)
            sortedList = addSortedElement(makeNewListElement(characters[i]), sortedList);
    }
    return sortedList;
}

// Adds elt to the sorted list head
list* addSortedElement(list* elt, list* head) {
    if (head == NULL)
        return elt;
    else if (head->elt->d.occurrences > elt->elt->d.occurrences) {
        elt->next = head;
        return elt;
    } else {
        head->next = addSortedElement(elt, head->next);
        return head;
    }
}

// Creates a new list from characterInfo
list* makeNewListElement(data characterInfo) {
    list* newElt = malloc(sizeof(list));
    newElt->elt = makeNewTreeElement(characterInfo);
    newElt->next = NULL;
    return newElt;
}

// Creates a new tree from element
Tree* makeNewTreeElement(data element) {
    Tree* newElt = malloc(sizeof(Tree));
    newElt->left = NULL;
    newElt->right = NULL;
    newElt->d = element;
    return newElt;
}

// Creates a new tree whose children are left and right
Tree* concatTree(Tree* left, Tree* right) {
    Tree* newElt = malloc(sizeof(Tree));
    newElt->left = left;
    newElt->right = right;
    newElt->d.character = 0;
    newElt->d.occurrences = left->d.occurrences + right->d.occurrences;
    return newElt;
}

// Prints the sorted list sortedList
void printSortedList(list* sortedList) {
    printf("sorted list\n");
    while (sortedList) {
        printf("character %c occurred %d times\n", sortedList->elt->d.character, sortedList->elt->d.occurrences);
        sortedList = sortedList->next;
    }
}

// Builds a tree by using the array characters containing nbCharacters
// The array represents the encoding hierarchy of characters
Tree* buildTree(data* characters, unsigned int nbCharacters) {
    list* sortedElements = buildSortedListFromArray(characters, nbCharacters);
    if (!sortedElements)
        return NULL;

    while (sortedElements->next) {
        Tree* left = sortedElements->elt;
        list* tmp = sortedElements;
        sortedElements = sortedElements->next;
        free(tmp);
        Tree* right = sortedElements->elt;
        tmp = sortedElements;
        sortedElements = sortedElements->next;
        free(tmp);
        list* elt = calloc(1, sizeof(list));
        elt->elt = concatTree(left, right);
        sortedElements = addSortedElement(elt, sortedElements);
    }
    Tree* result = sortedElements->elt;
    free(sortedElements);
    return result;
}

// Prints the tree root
void printTree(Tree* root) {
    if (root) {
        printf("Node occurrences %d\n", root->d.occurrences);
        printTree(root->left);
        printTree(root->right);
    }
}

// Frees the tree t
void freeTree(Tree* t) {
    if (t) {
        freeTree(t->left);
        freeTree(t->right);
        free(t);
    }
}

// Tries to write size characters from data into fd.
// Exists when encountering an error and prints the error message
void testWrite(unsigned int fd, void* data, unsigned int size) {
    if (write(fd, data, size) == -1) {
        perror("write");
        exit(1);
    }
}

// Tries to read size characters from fd into buffer.
// Exists when encountering an error and prints the error message
int testRead(unsigned int fd, size_t size, void* buffer) {
    int readCharacters = read(fd, buffer, size);
    if (readCharacters == -1) {
        perror("read");
        exit(1);
    }
    return readCharacters;
}