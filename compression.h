

typedef struct s_data 
{
    int size ; // size of the letter
    unsigned char letter; // code of the letter
    int occ;
} data;

typedef struct s_node
{
    int value; // "valeur" pour calculer l'importance
    data* d; // 
    struct s_node* first; // fils gauche
    struct s_node* second; // fils droit
} node;
typedef struct s_arbre 
{
    data* d;
    int value;
    struct s_arbre* left;
    struct s_arbre* right;
} arbre;

typedef struct s_hierarchy
{
    node* d;
    struct s_hierarchy* next;
} hierarchy;

typedef struct s_coded_letter
{
    unsigned char letter;
    char * code;
    int size;
} coded_letter;

typedef struct s_decompress_info
{
    coded_letter** tab;
    int nb_letters;
    int filling_last_byte;
} decompress_info;

typedef struct s_decompress_tree
{
    char letter;
    struct s_decompress_tree* left;
    struct s_decompress_tree* right;
} decompress_tree;
decompress_tree* add_elt_tree( char * code, char letter, decompress_tree*t);
void printByte( char c);


data ** count_occ ( data ** tab, int size,unsigned char *buffer);
arbre * make_arbre ( node * top);
hierarchy * add_elt( data* d, hierarchy *h);
hierarchy * add_node (node* n, hierarchy *h);
coded_letter** recursive_transform( arbre * a, coded_letter** tab, char * code, int size);
void free_nodes ( node * n);
char * concat( const char * S1, const char * S2);
int put_code_letter_to_buffer( unsigned char * buffer, int pos, coded_letter* l, int fd );
