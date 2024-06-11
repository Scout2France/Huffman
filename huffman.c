#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//PARTIE BUFFER
typedef struct InputBuffer {
    FILE *input_file;
    int file_end;
    uint8_t value;
    int bits_number;
    uint8_t next_value;
    uint8_t after_next_value;
} InputBuffer;

void init_InputBuffer(InputBuffer *buffer, FILE *file) {
    buffer->input_file = file;
    buffer->file_end = 0;
    buffer->value = 0;
    buffer->bits_number = 0;
    buffer->next_value = fgetc(buffer->input_file);
    if (buffer->next_value == EOF) {
        printf("Empty file.\n");
        buffer->file_end = 1;
    }
    buffer->after_next_value = fgetc(buffer->input_file);
}

void reset_InputBuffer(InputBuffer *buffer, FILE *file){
    init_InputBuffer(buffer, file);
}

int read_bit(InputBuffer *buffer){
    int result;
    if (buffer->bits_number <= 0) {
        if (buffer->file_end) {
            printf("Error: End of file reached.\n");
            return EOF;
        }
        buffer->value = buffer->next_value;
        buffer->next_value = buffer->after_next_value;
        buffer->bits_number = 8;
        buffer->after_next_value = fgetc(buffer->input_file);        
        if(feof(buffer->input_file)){
            buffer->file_end = 1;
            buffer->bits_number = buffer->next_value;
        }
    }
    result = buffer->value >> 7;
    buffer->value = (buffer->value & 127) << 1;
    buffer->bits_number--;
    return result;
}

uint8_t read_byte(InputBuffer *buffer) {
    uint8_t next_byte = 0;
    for (int i = 0; i < 8; i++) {
        int next_bit = read_bit(buffer);
        next_byte = (next_bit << (7 - i)) | next_byte;
    }
    return next_byte;
}

typedef struct OutputBuffer {
    FILE *output_file;
    uint8_t value;
    int bits_number;
} OutputBuffer;

void init_OutputBuffer(OutputBuffer *buffer, FILE *file) {
    buffer->output_file = file;    
    buffer->value = 0;
    buffer->bits_number = 0;
}

void reset_OutputBuffer(OutputBuffer *buffer, FILE *file) {
    init_OutputBuffer(buffer, file);
}

void write_bit(OutputBuffer *buffer, int bit) {    
    buffer->value = (buffer->value << 1) | (bit & 1);
    buffer->bits_number++;
    if (buffer->bits_number >= 8) {
        fputc(buffer->value, buffer->output_file);
        reset_OutputBuffer(buffer, buffer->output_file);
    }    
}

void write_byte(OutputBuffer *buffer, uint8_t next_byte) {       
    if(next_byte == 130){      
        write_byte(buffer,195);
        write_byte(buffer,169);
    }
    else{
    uint8_t tmp = next_byte;
    for (int i = 0; i < 8; i++) {
        int bit = tmp >> 7;
        write_bit(buffer, bit);
        tmp = (tmp << 1) & 0xFF;
    }
    }
}

void finish(OutputBuffer *buffer){    
    if (buffer->bits_number > 0) {
        buffer->value <<= (8 - buffer->bits_number);
        fputc(buffer->value, buffer->output_file);
    }
    if (buffer->bits_number == 0) {
        buffer->bits_number = 8;
    }
    fputc(buffer->bits_number, buffer->output_file);
}

//IMPLEMENTATION DE LA STRUCTURE DE TAS ET D'ARBRE DE HUFFMAN
typedef struct Node {int frequency ; int value ; struct Node *fg ; struct Node *fd;} Node;
typedef struct heap { struct heap *fg ; Node *noeud ; struct heap *fd;} heap;

void ajout(heap **t, Node *a){            
    if (*t == NULL) {
        *t = (heap*)malloc(sizeof(heap));        
        (*t)->fg = NULL;
        (*t)->noeud = a;
        (*t)->fd = NULL;        
    } else {
        if ((*a).frequency < (*t)->noeud->frequency) {
            heap *temp1 = (*t)->fg;
            Node *temp2 = malloc(sizeof(Node));
            temp2 = (*t)->noeud;
            (*t)->fg = (*t)->fd;
            (*t)->noeud = a;
            (*t)->fd = temp1;
            ajout(&((*t)->fd), temp2);
        }
        else{
            heap *temp = (*t)->fg;
            (*t)->fg = (*t)->fd;
            (*t)->fd = temp;
            ajout(&((*t)->fd), a);
        }
    }
}

void ajoutListe(heap **t, Node **liste, unsigned int taille){
    for(unsigned int i=0;i<taille;i++){
        ajout(t,liste[i]);
    }
}

void extraireMin(heap **t){
    if((*t)->fg == NULL && (*t)->fd == NULL){
        *t = NULL;
    }
    else if((*t)->fd == NULL){
        *t = (*t)->fg;
    }
    else if((*t)->fg == NULL){
        *t = (*t)->fd;
    }
    else{
        Node *ag = (*t)->fg->noeud;
        Node *ad = (*t)->fd->noeud;
        if((*ag).frequency < (*ad).frequency){
            (*t)->noeud = ag;
            extraireMin(&((*t)->fg));
        }
        else{
            (*t)->noeud = ad;
            extraireMin(&((*t)->fd));
        }
    } 
}

Node *extraireMinElement(heap **t){
    Node *res = (*t)->noeud;
    extraireMin(t);
    return res;
}

Node *creationArbre(int f, int v){    
    Node *arbre = malloc(sizeof(Node));
    arbre->frequency = f;
    arbre->value = v;
    arbre->fg = NULL;
    arbre->fd = NULL;
    return arbre;
}

Node *fusion(Node *a1, Node *a2){
    Node *arbre = malloc(sizeof(Node));
    arbre->frequency = (*a1).frequency+(*a2).frequency;
    arbre->value = -1;
    arbre->fg = a1;
    arbre->fd = a2;    
    return arbre;
}

int *countFrequence(FILE *input_stream){
    int *res = malloc(sizeof(int)*256);
    for(int i=0;i<256;i++){
        res[i] = 0;
    }
    int charactere;
    while(charactere != EOF){
        charactere = fgetc(input_stream);
        res[charactere]++;
    }    
    return res;
}

void fill_up(char *temp,Node *arb,char **tab){    
    if((*arb).fg == NULL && (*arb).fd == NULL){        
        tab[(*arb).value] = malloc(strlen(temp));        
        strcpy(tab[(*arb).value],temp);
    }
    else{
        int taille = strlen(temp);
        char *temp1 = malloc(taille+1);
        char *temp2 = malloc(taille+1);        
        strcpy(temp1,temp);
        strcpy(temp2,temp);
        temp1[taille] = '0';
        temp1[taille+1] = '\0';
        temp2[taille] = '1';
        temp2[taille+1] = '\0';    
        fill_up(temp1,(*arb).fg,tab);
        fill_up(temp2,(*arb).fd,tab);
    }
}

Node *huffman_tree_huffman(int *frequency){
    unsigned int compteur = 0;
    for(int i=0;i<256;i++){
        if(frequency[i] != 0){
            compteur++;
        }
    }
    Node **treesList = malloc(compteur*(sizeof(Node)+sizeof(int)));
    compteur = 0;
    for(int i=0;i<256;i++){
        if(frequency[i] != 0){
            treesList[compteur] = creationArbre(frequency[i],i);            
            compteur++;
        }
    }    
    heap *tas = NULL;
    ajoutListe(&tas,treesList,compteur);
    Node *min1;
    Node *min2;
    for(unsigned int i=0;i<compteur-1;i++){
        min1 = extraireMinElement(&tas);        
        min2 = extraireMinElement(&tas);
        ajout(&tas,fusion(min1,min2));
    }
    return extraireMinElement(&tas);
}

char **code_compression(Node *arbreHuffman){
    char **code_table = malloc(sizeof(char*)*256);
    for(int i=0;i<256;i++){
        code_table[i] = malloc(sizeof(char));        
        code_table[i][0] = '\0';
    }
    char *temp = malloc(sizeof(char));    
    temp[0] = '\0';    
    fill_up(temp,arbreHuffman,code_table);
    return code_table;
}

void huffman_tree_to_compress_file(OutputBuffer *output_buffer, Node *arbreHuffman){
    if((*arbreHuffman).fg == NULL && (*arbreHuffman).fd == NULL){
        write_bit(output_buffer,0);
        write_byte(output_buffer,(*arbreHuffman).value);
    }
    else{
        write_bit(output_buffer,1);
        huffman_tree_to_compress_file(output_buffer,(*arbreHuffman).fg);
        huffman_tree_to_compress_file(output_buffer,(*arbreHuffman).fd);
    }
}

void compression(FILE *input_stream, OutputBuffer *output_buffer, char **code_table){
    while(1){
        int character = fgetc(input_stream);
        if(character == EOF){
            break;
        }        
        int i=0;
        while(code_table[character][i] != '\0'){             
            write_bit(output_buffer,code_table[character][i]-'0');
            i++;
        }
    }
    finish(output_buffer);
}

void file_compression_wrapper(char *input_file_name, char *output_file_name){
    FILE *input_stream = NULL;
    input_stream = fopen(input_file_name,"r");
    FILE *output_value = NULL;
    output_value = fopen(output_file_name,"wb");
    OutputBuffer output_buffer ;
    init_OutputBuffer(&output_buffer,output_value);
    int *tab = countFrequence(input_stream);
    Node *huffman_tree = huffman_tree_huffman(tab);
    char **tab_code = code_compression(huffman_tree);
    fseek(input_stream,0,SEEK_SET);
    huffman_tree_to_compress_file(&output_buffer,huffman_tree);    
    compression(input_stream,&output_buffer,tab_code);
    fclose(input_stream);
    fclose(output_value);
}

Node *building_back_huffman_tree(InputBuffer *input_buffer){
    Node *res = malloc(sizeof(Node));
    int character = read_bit(input_buffer);
    if(character==0){
        int character2 = read_byte(input_buffer);        
        res = creationArbre(0,character2);
        return res;
    }
    Node *left = malloc(sizeof(Node));
    Node *right = malloc(sizeof(Node));
    left = building_back_huffman_tree(input_buffer);
    right = building_back_huffman_tree(input_buffer);
    res = fusion(left,right);
    return res;
}

void decompression(InputBuffer *input_buffer,OutputBuffer *output_buffer, Node *huffman_tree){
    Node *node = huffman_tree;
    int nb_bits = 10;    
    int current_bit;
    while(nb_bits>0){                
        if((*node).fg == NULL && (*node).fd == NULL){              
            write_byte(output_buffer,(*node).value);
            node = huffman_tree;            
        }
        else{
            if((*input_buffer).file_end && nb_bits == 10){
                current_bit = read_bit(input_buffer);                
                nb_bits = (*input_buffer).bits_number + 1;
            }
            else{
                current_bit = read_bit(input_buffer);                
                if((*input_buffer).file_end && nb_bits == 10){
                    nb_bits = (*input_buffer).bits_number + 1;
                }    
            }
            if(current_bit == 0){
                node = (*node).fg;
            }
            else{
                if(current_bit == 1){
                    node = (*node).fd;
                }
            }
            if(nb_bits<10){
                nb_bits--;
            }
        }
    }    
    if((*node).fg == NULL && (*node).fd == NULL){        
        write_byte(output_buffer,(*node).value);
    }
}

void file_decompression_wrapper(char *input_file_name, char *output_file_name){
    FILE *input_stream = NULL;
    input_stream = fopen(input_file_name,"rb");
    FILE *output_value = NULL;
    output_value = fopen(output_file_name,"wb");
    InputBuffer input_buffer ;
    init_InputBuffer(&input_buffer,input_stream);
    OutputBuffer output_buffer ;
    init_OutputBuffer(&output_buffer,output_value);
    Node *res = building_back_huffman_tree(&input_buffer);
    decompression(&input_buffer,&output_buffer,res);
    fclose(input_stream);
    fclose(output_value);
}

int main(int argc, char *argv[]){    
    if(argv[1][0] == '-' && argv[1][1] == 'c'){
        file_compression_wrapper(argv[2],argv[3]);
    }else{
        if(argv[1][0] == '-' && argv[1][1] == 'd'){
            file_decompression_wrapper(argv[2],argv[3]);
        }
    }
    return 0;
}
