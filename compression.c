#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "compression.h"

#define BUFFER_SIZE 512



data** read_file( char* filename )
{
    unsigned char buffer[BUFFER_SIZE];
    int size = 0, fd,i;
    data ** tab = malloc (sizeof(data*)*256); // tableau for each character
    for (i= 0 ; i < 255;i++)
        tab[i] = NULL;
    if ( (fd = open( filename, O_RDONLY, 0666)) == -1)
    {
        perror("open");
        exit(1);
    }
    
    while ( (size = read( fd, buffer, BUFFER_SIZE)) == BUFFER_SIZE )
    {
        tab = count_occ ( tab,size,buffer);
    }
    if(size==-1)
    {
        perror("read");
        exit(1);
    }
    tab = count_occ ( tab,size,buffer);
    close(fd);
    return tab;
}

int compress_file ( char * filename, coded_letter ** tab)
{
    unsigned char buffer_read[BUFFER_SIZE],buffer_write[BUFFER_SIZE];
    int size = 0, fd_read, fd_write,i;
    int pos = 0, nb_letters = 0;
    char * compress_file_name =concat(filename,"_compress");

    if ( (fd_read = open( filename, O_RDONLY, 0666)) == -1)
    {
        perror("open");
        exit(1);
    }
    if ( (fd_write = open( compress_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
    {
        perror("open");
        exit(1);
    }
    
    for(i=0; i < 255 ; i++)
    {
        if(tab[i] != NULL)
            nb_letters++;
    }
    
    if ( write(fd_write, &nb_letters , sizeof(int)) ==-1)
    {
        perror("write");
        exit(1);
    }
    
    if ( lseek( fd_write, 4, SEEK_CUR ) == -1 ) // move cursor to make room for number of bits filled in last byte
    {
        perror("lseek");
        exit(1);
    }
    
    for(i=0 ; i< 255 ; i++)
    {
        if(tab[i] != NULL)
        {
            if ( write(fd_write, &tab[i]->letter, sizeof(char)) == -1)
            {
                perror("write");
                exit(1);
            }
            if ( write(fd_write, &tab[i]->size, sizeof(int)) == -1)
            {
                perror("write");
                exit(1);
            }
            if ( write(fd_write, tab[i]->code, tab[i]->size) == -1)
            {
                perror("write");
                exit(1);
            }
        }
    }
    
    for(i=0; i < BUFFER_SIZE ; i++) // clear buffer
        buffer_write[i] = 0;


    while ( (size = read( fd_read, buffer_read, BUFFER_SIZE)) != 0 )
    {
        for(i =0 ; i < size;i ++)
            pos = put_code_letter_to_buffer( buffer_write, pos, tab[(unsigned int)buffer_read[i]], fd_write );
    }

    // write the rest of the buffer to file
    if ( write( fd_write, buffer_write, pos / 8 + 1) == -1)
    {
        perror("write");
        exit(1);
    }
    
    // write the number of bits used in the last Byte
    if( lseek(fd_write, 4, SEEK_SET ) == -1)
    {
        perror("lseek");
        exit(1);
    }
    pos = pos%8;
    
    if (write( fd_write, &pos , sizeof(int)) == -1)
    {
        perror("write");
        exit(1);
    }
    
    close(fd_read);
    close(fd_write);
    free(compress_file_name);
    return 1;
}


// count the occ of character in the text
data ** count_occ ( data ** tab, int size,unsigned char *buffer)
{
    int i, index;
    for (i=0 ; i < size ; i++)
    {
        index = buffer[i];
        if( tab[index] == NULL )
        {
            tab[index] = malloc (sizeof( data) );
            tab[index]->occ = 1;
            tab[index]->letter = index;
        }
        else
            tab[buffer[i]]->occ += 1;
        
    }
    return tab;
}


// Make tree for Huffman codage
arbre *make_tree( data ** tab)
{
    int i =0;
    hierarchy * top = NULL; 
    hierarchy * copie;
    node * first, * second, * n;
    
    for (i = 0;i < 255 ; i ++) // begin with create a sorted chained list which contains all characters present in the file
    {
        if (tab[i] != NULL)
            top = add_elt ( tab[i], top );
    }
    
    // build the tree
    while ( top->next != NULL )
    {
        first= top->d;
        copie = top;
        top = top->next;
        free(copie);
        copie = top;
        second= top->d;
        top = top->next;
        free(copie);
        n = malloc(sizeof(node));
        n->first = first; n->second = second;
        n->value = first->value + second->value;
        n->d = NULL;
        top = add_node( n , top);
    }
    
    // make tree with new structure
    arbre* a= make_arbre(top->d);
    free_nodes(top->d);
    free(top);
    return a;
}

// free nodes tree
void free_nodes ( node * n)
{
    if( n->first != NULL && n->second != NULL)
    {
        free_nodes(n->first);
        free_nodes(n->second);
    }
    free(n);
}

// make tree with arbre structure
arbre * make_arbre ( node * top)
{
    arbre * e;
    if( top->first == NULL && top->second == NULL ) 
    {
        e = malloc( sizeof(arbre ));
        e->left = NULL;
        e->right = NULL;
        e->value = top->value;
        e->d = top->d;
    }
    else
    {
        e = malloc( sizeof(arbre));
        e->left = make_arbre(top->first);
        e->right = make_arbre(top->second);
        e->value = top->value;
    }
    return e;
}

// print tree (debug)
void show_tree( arbre * a,int niveau)
{
    int i;
    if(a== NULL)
        return;
    for (i= 0;i< niveau; i++)
        printf("\t");
    if(a->left == NULL && a->right == NULL)
    {
        printf("Node Val %d.",a->value);
        printf(" Letter %c\n",a->d->letter);
    }
    else
        printf("Node Val %d\n",a->value);

    niveau++;
    show_tree( a->left,niveau);
    show_tree( a->right,niveau);
    
}

// add elt to chained list
hierarchy * add_elt( data* d, hierarchy *h)
{
    hierarchy * top = h;
    if (top == NULL)
    {
        top = malloc (sizeof(hierarchy));

        node *f = malloc(sizeof(node));
        f->first = NULL; f->second = NULL;
        f->d = d;
        f->value = d->occ;
        top->d = f;
        top->next = h;
    }
    else if ( top->d->d->occ >= d->occ )
    {
        top = malloc (sizeof(hierarchy));
        node *f = malloc(sizeof(node));
        f->first = NULL; f->second = NULL;
        f->d = d;
        f->value = d->occ;
        top->d = f;
        top->next = h;
    }
    else
        top->next = add_elt( d, h->next) ; 
    return top;
}

// add a node to chained list
hierarchy * add_node (node* n, hierarchy *h)
{
    hierarchy * top = h;
    if ( h ==NULL )
    {
        top = malloc (sizeof(hierarchy));
        top->d = n;
        top->next = h;
    }
    else if ( top->d->value >= n->value )
    {
        top = malloc (sizeof(hierarchy));
        top->d = n;
        top->next = h;
    }
    else
    {
        top->next = add_node( n, top->next) ;
    }
    return top;
}

// transform tree in coded_letter tab
coded_letter ** transform_tree( arbre *a)
{
    int i;
    coded_letter ** tab = malloc(255 * sizeof(coded_letter));
    for(i=0; i< 255; i++)
        tab[i] = NULL;
    char * begin = malloc(1);
    begin[0] = 0 ;
    tab = recursive_transform(a,tab,begin,1);
    return tab;    
}

// recursive function which serve to build the tree in transform_tree
coded_letter** recursive_transform( arbre * a, coded_letter** tab, char * code, int size)
{
    coded_letter * l;
    if( a->left == NULL && a->right == NULL) // atteint une feuille
    {
        l = malloc( sizeof(coded_letter));
        l->letter = a->d->letter;
        l->size = size;
        l->code = code;
        tab[ (int)l->letter] = l;
    }
    else
    {
        tab = recursive_transform( a->left, tab, concat(code,"0"), size+1);
        tab = recursive_transform( a->right, tab, concat(code,"1"), size+1);
        free(code);
    }
    return tab;
}

// free the tree
void free_tree( arbre *a)
{
    if( a->left == NULL && a->right == NULL)
        free(a);
    else if( a->left == NULL )
    {
        free_tree(a->right);
        free(a);
    }
    else if( a->right == NULL )
    {
        free_tree(a->left);
        free(a);
    }
    else
    {
        free_tree(a->left);
        free_tree(a->right);
        free(a);
    }
}

// concat 2 string
char * concat( const char * S1, const char * S2)
{
    char * res;
    int pos = 0, size1 = 0, size2 = 0;
    while( S1[size1] != 0 )
        size1++;
    while( S2[size2] != 0 )
        size2++;
    
    res = malloc( size1 + size2 + 1);
    while (pos < size1)
    {
        res[pos] = S1[pos];
        pos ++;
    }
    while (pos - size1  < size2)
    {
        res[pos ] = S2[pos - size1 ];
        pos ++;
    }
    res[pos] = 0;
    return res;    
}

// foncton which put a letter into the buffer
// return the pos which is counted IN BIT AND NOT BYTE
int put_code_letter_to_buffer( unsigned char * buffer, int pos, coded_letter* l, int fd)
{
    int  i;
    char * letter;
    // if buffer is full then write to file
    if( pos + l->size > BUFFER_SIZE*8 )
    {
        if (write( fd, buffer, pos/8 ) == -1)
        {
            perror("write");
            exit(1);
        }
        for(i=0; i < BUFFER_SIZE - pos/8 ; i++)
            buffer[i] = buffer[ pos/8+i ];
        pos = (BUFFER_SIZE - pos/8 - 1) * 8 + (pos %8) ;
        
        // clear the rest of the last Byte and clear the rest of the buffer 
        for(i= pos % 8 ; i < 8 ; i++)
            buffer[pos/8]  = buffer[pos/8] & ~(1 << i);

        for(i= pos/8+1 ; i < BUFFER_SIZE ; i++ )
            buffer[i] = 0;
    }
    letter = l->code;
    while( *letter != 0)
    {
        if( *letter == '1')
        {
            buffer[pos/8] |= 1 << pos%8;
            pos ++;
        }
        else
            pos++;
        letter ++;
    }    
    return pos;
}

int main (int argc, char* argv[])
{
    int i;
    if( argc != 2)
    {
        printf("usage %s: <filename>\n",argv[0]);
        exit(1);
    }
    data**tab = read_file (argv[1]);
    /*for (i=0 ; i< 255; i ++)
    {
        if( tab[i] != NULL)
            printf("occurence pour le caractère %c est %d\n",i, tab[i]->occ);
    }
    */
    arbre * a = make_tree(tab);    
    //show_tree(a,0);
    coded_letter ** letter_tab = transform_tree(a);
    for (i= 0 ; i< 255;i ++)
    {
        if(letter_tab[i] != NULL)
            printf("La lettre %c est codée par %s\n",letter_tab[i]->letter, letter_tab[i]->code);
    }
    compress_file( argv[1], letter_tab);
    free_tree(a);
    for(i=0; i< 255 ;i ++)
    {
        if( tab[i] )
            free(tab[i]);
        if(letter_tab[i] ) 
        {
            free(letter_tab[i]->code);
            free(letter_tab[i]);
        }
    }
    free(tab);
    free(letter_tab);
    return 0;
}
