#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "compression.h"

#define BUFFER_SIZE 512


// read file and take infos
decompress_info read_infos(int fd)
{
    int nb_letters,i, filling_last_byte;
    coded_letter** tab ;
    decompress_info info;
    
    if( read( fd, &nb_letters, sizeof(int)) == -1)
    {
        perror("read");
        exit(1);
    }
    if( read( fd, &filling_last_byte, sizeof(int)) == -1)
    {
        perror("read");
        exit(1);
    }
    
    tab = malloc(nb_letters* sizeof(coded_letter*));
    for(i=0; i< nb_letters; i++)
    {
        tab[i] = malloc(sizeof(coded_letter));
        if( read( fd, &tab[i]->letter, sizeof(char)) == -1)
        {
            perror("read");
            exit(1);
        }
        if( read( fd, &tab[i]->size, sizeof(int)) == -1)
        {
            perror("read");
            exit(1);
        }
        tab[i]->code = malloc(tab[i]->size);
        if( read( fd, tab[i]->code, tab[i]->size) == -1)
        {
            perror("read");
            exit(1);
        }
    }
    info.nb_letters = nb_letters;
    info.tab = tab;
    info.filling_last_byte =filling_last_byte;
    return info;
}

// sort tab depending on the size of coded letter
coded_letter** tri( coded_letter** tab,int size)
{
    int i,j;
    coded_letter ** new_tab = malloc(size * sizeof(coded_letter*));
    coded_letter * min;
    int pos, size2=size;
    for(i=0 ; i < size2 ; i++)
    {
        min = tab[0];
        pos = 0;
        for(j=1; j < size ; j++)
        {
            if( min->size > tab[j]->size)
            {
                min = tab[j];
                pos = j;
            }
        }
        tab[pos] = tab[size-1];
        size --;
        new_tab[i] = min;
    }
    free(tab);
    return new_tab;
}

// make the tree
decompress_tree* tree ( coded_letter** tab,int size)
{
    int i;
    decompress_tree * top = malloc(sizeof(decompress_tree));
    top->left = NULL;
    top->right = NULL;
    for (i=0;i< size; i++)
        top = add_elt_tree(tab[i]->code, tab[i]->letter,top);
    return top;
}

// add elt to tree
decompress_tree* add_elt_tree( char * code, char letter, decompress_tree*t)
{
    if (*code == 0)
    {
        t= malloc(sizeof(decompress_tree));
        t->letter = letter;
        t->right = NULL;
        t->left = NULL;
    }
    else
    {
        if (t==NULL)
        {
            t= malloc(sizeof(decompress_tree));
            t->right = NULL;
            t->left = NULL;
            t->letter =0;
        }
        if(code[0] == '0')
            t->left = add_elt_tree(++code,letter, t->left);
        else
            t->right = add_elt_tree(++code,letter, t->right);
    }
    return t;
}


// free the tree
void free_tree( decompress_tree *t)
{
    if(t->right != NULL)
        free_tree(t->right);
    if (t->left != NULL)
        free_tree(t->left);
    free(t);
}

// return value of bit bit_num
int bit_val ( char octet, int bit_num)
{
    return ( octet & ( 1 << (bit_num -1))) != 0;
}


void decompress( decompress_tree* t, int fd_read, decompress_info info)
{
    char buffer_read[BUFFER_SIZE], buffer_write[BUFFER_SIZE];
    int  test, pos_r, pos_w=0, fd_write ;
    decompress_tree * copie_t= t;
    int false_end_nb_letters = 0, i;
    off_t size_of_file;
    
    if ( (fd_write = open( "decompressed_file", O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
    {
        perror("open");
        exit(1);
    }
    while ( (test=read( fd_read, buffer_read, BUFFER_SIZE)) != 0)
    {
        if( test == -1)
        {
            perror("read");
            exit(1);
        }
        pos_r =0;
        while( pos_r < test*8)
        {
            if( bit_val( buffer_read[ pos_r/8 ], pos_r % 8 +1) )
                copie_t = copie_t->right;
            else
                copie_t = copie_t->left;

            pos_r ++;
            if( copie_t->right == NULL && copie_t->left == NULL ) // leaf -> a letter is decoded
            {
                buffer_write[pos_w] = copie_t->letter;
                pos_w ++;

                if( pos_w == BUFFER_SIZE )
                {
                    if ( write(fd_write, buffer_write, BUFFER_SIZE) == -1)
                    {
                        perror("write");
                        exit(1);
                    }
                    pos_w = 0;
                }
                copie_t = t;
            }
        }
    }

    if ( write(fd_write, buffer_write, pos_w) == -1)
    {
        perror("write");
        exit(1);
    }

    // Now count number of letters which are decoded mistakenly
    for(i = info.filling_last_byte ; i < 8 ; i ++)
    {
        if( bit_val( buffer_read[ pos_r/8 ], pos_r % 8 +1) )
            copie_t = copie_t->right;
        else
            copie_t = copie_t->left;
        
        if( copie_t->right == NULL && copie_t->left == NULL ) // leaf -> a letter is decoded
        {
            false_end_nb_letters ++;
            copie_t = t;
        }
    }
    
    if( ( size_of_file = lseek( fd_write, 0, SEEK_END ) ) == -1)
    {
        perror("lseek");
        exit(1);
    }
    if( ftruncate( fd_write , size_of_file - false_end_nb_letters) == -1) // remove last false_end_nb_letters from file
    {
        perror("truncate");
        exit(1);
    } 
}

void printByte( char c)
{
    printf("%d %d %d %d %d %d %d %d\n",bit_val(c,1),bit_val(c,2),bit_val(c,3),bit_val(c,4),bit_val(c,5),bit_val(c,6),bit_val(c,7),bit_val(c,8));
    
}

int main(int argc, char* argv[])
{
    if( argc != 2)
    {
        printf("usage %s: <filename>\n",argv[0]);
        exit(1);
    }
    int i, fd;
    decompress_info info;

    if ( (fd = open( argv[1], O_RDONLY, 0666)) == -1)
    {
        perror("open");
        exit(1);
    }
    info = read_infos(fd);
    
    decompress_tree* t = tree( info.tab, info.nb_letters);
    // info are taken, now read the compressed data and decompress it
    decompress( t, fd,info);

    for(i=0; i < info.nb_letters ;i++)
    {
        free(info.tab[i]->code);
        free(info.tab[i]);
    }
    free(info.tab);
    free_tree(t);
    return 0;
}
