/****************************************************************************
 * Nome: auxiliar.h
 * Autor: André Akira
 * --------------------------------------------------------------------------
 * 
 * Este arquivo define as funções reponsáveis por auxiliar na manipulação
 * dos dados dos membros no archive utilizado pelo compilador VINAc.
 * 
 * *************************************************************************/

#ifndef AUXILIAR_T
#define AUXILIAR_T

struct Directory;

// retorna o maior tamanho a ser utilzado pelo buffer
size_t max_tam_buffer(struct Directory *dir, size_t tam_atual);

// atualiza o offset dos membros afetados de acordo com deslocamento inidicado
void fix_member_offset(struct Directory *dir, unsigned int index, unsigned int tam_final, long deslocamento);

// cria nomes para os arquivos na hora de extrair, evitando arquivos com nomes duplicados
void create_name(const char *nome_original, char *nome_final, size_t tam_buffer);

// desloca quantidades de dados no archive
int move_data(FILE *arch, long origem, long destino, size_t tam_dados, size_t buffer_size);

#endif