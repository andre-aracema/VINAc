/****************************************************************************
 * Nome: achive.h
 * Autor: André Akira
 * --------------------------------------------------------------------------
 * 
 * Este arquivo define as funções reponsáveis pela manipulação
 * dos dados dos membros no archive utilizado pelo compilador VINAc.
 * 
 * O archive é a área onde armazena o diretório e os dados dos membros.
 * 
 * As funções permitem realizar as seguintes operações:
 * (-p) inserção sem compressão: insere/acrescenta um ou mais membros sem 
 * compressão no archive. 
 * (-i) inserção com compressão: insere/acrescenta um ou mais membros com
 * compressão no archive.
 * (-m) move: move o membro indicado para imidiatamente depois do membro
 * target existente no archive.
 * (-x) extrai: extrai os membros indicados de archive.
 * (-r) remove: remove os membros indicados do archive.
 * (-c) lista: lista o conteúdo de archove em ordem, incluido as propriedades
 * de cada membro.
 * 
 * *************************************************************************/

#ifndef ARCHIVE_T
#define ARCHIVE_T

struct Directory;

/* insere um novo membro sem compressão no archive 
*  ou reescreve um arquivo existente de mesmo nome */
int insert_member(FILE *arch, const char *nome_membro);

/* insere um novo membro com compressão no archive 
*  ou reescreve um arquivo existente de mesmo nome */
int insert_member_compressed(FILE *arch, const char *nome_membro);

// move um membro para logo após o membro_target no archive
int move_member(FILE *arch, const char *nome_membro, const char *nome_membro_target);

// extrai membro do archive
int extract_members(FILE *arch, const char *nome_membro);

// remove um membro do archive
int remove_member(FILE *arch, const char *nome_membro);

// lista ('imprime') os metadados dos membros presentes
int list_members(struct Directory *dir);

#endif