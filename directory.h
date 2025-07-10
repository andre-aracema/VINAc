/****************************************************************************
 * Nome: directory.h
 * Autor: André Akira
 * --------------------------------------------------------------------------
 * 
 * Este arquivo define as estruturas e funções reponsáveis pela
 * manipulação do diretório utilizado pelo compilador VINAc.
 * 
 * O diretório é a área do archive que armazena os metadados de
 * todos os membros do arquivo, como o nome, UID (User Identifier), 
 * tamanho original, tamanho em disco, data de modificação, ordem no 
 * arquivo e localização (offset a partir do início).
 * 
 * A estrutura 'MemberInfo' representa os metadados dos membros do
 * archive, enquanto a estrutura 'Directory' representa o diretório
 * em si, contendo informações como o número de membros, a localização
 * final do diretório e o conjunto completo dos membros.
 * 
 * As funções permitem a leitura e escrita do diretório, bem como operações 
 * de inserção, remoção e de deslocamento dos membros no diretório, sendo
 * fundamentais para o funcionamento do archive.
 * 
 * *************************************************************************/

#ifndef DIRECTORY_T
#define DIRECTORY_T

#define MAX_LEN_NAME 128
#define OFFSET_START_DIRECTORY 0

// estrutura responsável por guardar os metadados dos membros
struct MemberInfo{
    char nome[MAX_LEN_NAME];
    uid_t uid;
    size_t tam_original;
    size_t tam_disco;
    time_t mod_data;
    unsigned int ordem;
    size_t offset;
    int comprimido;
};

// estrutura responsável por representar o diretório
struct Directory{
    struct MemberInfo *membros;
    unsigned int tam_membros;
    size_t offset_fim_diretorio;
};


// funções reponsáveis por manipular o diretorio

// lê em disco o conteúdo do diretório
int read_directory(FILE *archive, struct Directory *dir);

// escreve em disco o conteúdo do diretório
int write_directory(FILE *archive, struct Directory *dir);

// insere os metadados dos membros no diretório
int initialize_member_directory(struct Directory *dir, unsigned int index, const char *nome, uid_t uid, size_t tam_originial, size_t tam_disco, time_t mod_data, unsigned int ordem, size_t offset, int comprimido);

/* retorna a posição do membro se existir;
*  caso contrário retorna -1; */
int find_member_directory(struct Directory *dir, const char *nome_membro);

// desloca o membro para logo após o membro_target no diretório
int move_member_directory(struct Directory *dir, int index_member, int index_target);

// remove o membro do diretório
int remove_member_directory(struct Directory *dir, const char *nome_membro);

// insere o membro no diretório
int insert_member_directory(struct Directory *dir, const char *nome_arquivo, size_t tam_original, size_t tam_disco, size_t offset, int comprimido);

#endif