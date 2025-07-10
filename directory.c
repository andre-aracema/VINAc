#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "directory.h"
#include "archive.h"

int read_directory(FILE *archive, struct Directory *dir){

    fseek(archive, OFFSET_START_DIRECTORY, SEEK_SET);
    
    // lendo offset do fim do diretório
    fread(&dir->offset_fim_diretorio, sizeof(size_t), 1, archive);

    size_t tam_dir = dir->offset_fim_diretorio;

    // possuí membros
    if(tam_dir > sizeof(size_t)){

        dir->tam_membros = (unsigned int)((tam_dir - sizeof(size_t))/sizeof(struct MemberInfo));
    
        dir->membros = (struct MemberInfo *)malloc(sizeof(struct MemberInfo) * dir->tam_membros);
        if (dir->membros == NULL){
            return -1;
        }

        // lendo membros (metadados)
        for(unsigned int i = 0; i < dir->tam_membros; i++){
            fread(&dir->membros[i], sizeof(struct MemberInfo), 1, archive);
        }
    }else{
        dir->membros = NULL;
        dir->tam_membros = 0;
    }

    return 0;
}

int write_directory(FILE *archive, struct Directory *dir){

    fseek(archive, OFFSET_START_DIRECTORY, SEEK_SET);
    
    // atualizando offset do fim do diretório
    dir->offset_fim_diretorio = OFFSET_START_DIRECTORY + sizeof(size_t) + dir->tam_membros * sizeof(struct MemberInfo);

    // escreve o offset
    fwrite(&dir->offset_fim_diretorio, sizeof(size_t), 1, archive);

    // possuí membros
    if(dir->tam_membros > 0){

        // escreve membros (metadados)
        for(unsigned int i = 0; i < dir->tam_membros; i++){
            fwrite(&dir->membros[i], sizeof(struct MemberInfo), 1, archive);
        }
    }

    return 0;
}

int initialize_member_directory(struct Directory *dir, unsigned int index, const char *nome, uid_t uid, size_t tam_originial, size_t tam_disco, time_t mod_data, unsigned int ordem, size_t offset, int comprimido){

    struct MemberInfo *new = &dir->membros[index];

    strncpy(new->nome, nome, MAX_LEN_NAME - 1);
    new->nome[MAX_LEN_NAME - 1] = '\0';

    new->uid = uid;
    new->tam_original = tam_originial;
    new->tam_disco = tam_disco;
    new->mod_data = mod_data;
    new->ordem = ordem;
    new->offset = offset;
    new->comprimido = comprimido; // auxiliar

    return 0;
}

int find_member_directory(struct Directory *dir, const char *nome_membro){

    for(unsigned int i = 0; i < dir->tam_membros; i++){

        // compara nomes
        if(strcmp(dir->membros[i].nome, nome_membro) == 0){
            return (int)i;
        }
    }

    return -1;
}

int move_member_directory(struct Directory *dir, int index_member, int index_target){

    if(index_member == index_target){
        return 0;
    }
    
    struct MemberInfo temp = dir->membros[index_member];

    // deslocamento para esquerda (sobrescrita do membro a ser movido)
    if(index_member < index_target){
        for(int i = index_member; i < index_target; i++){
            dir->membros[i] = dir->membros[i + 1];
        }

        // atualizando posição do membro
        dir->membros[index_target] = temp;
    }
    
    // deslocamento para direita (sobrescrita do membro a ser movido)
    else{
        for(int i = index_member; i > index_target + 1; i--){
            dir->membros[i] = dir->membros[i - 1];
        }

        // atualizando posição do membro
        dir->membros[index_target + 1] = temp;
    }

    // atualiza ordem
    for(unsigned int i = 0; i < dir->tam_membros; i++){
        dir->membros[i].ordem = i + 1;
    }

    return 0;
}

int remove_member_directory(struct Directory *dir, const char *nome_membro){

    int index = find_member_directory(dir, nome_membro);
    if(index == -1){
        return -1;
    }

    // sobrescrevendo
    for(unsigned int i = (unsigned int)index; i < dir->tam_membros - 1; i++){
        dir->membros[i] = dir->membros[i + 1];
    }

    dir->tam_membros--;
    dir->membros = (struct MemberInfo *)realloc(dir->membros, sizeof(struct MemberInfo) * dir->tam_membros);
    if(dir->membros == NULL && dir->tam_membros > 0){
        return -1;
    }

    // atualizando ordem
    for (unsigned int i = 0; i < dir->tam_membros; i++){
        dir->membros[i].ordem = i + 1;
    }

    return 0;
}

int insert_member_directory(struct Directory *dir, const char *nome_membro, size_t tam_original, size_t tam_disco, size_t offset, int comprimido){

    struct stat mi;
    if(stat(nome_membro, &mi) != 0){
        return -1;
    }

    dir->tam_membros++;
    dir->membros = (struct MemberInfo *)realloc(dir->membros, sizeof(struct MemberInfo) * dir->tam_membros);
    if(dir->membros == NULL){
        return -1;
    }

    unsigned int index = dir->tam_membros - 1;
    
    initialize_member_directory(dir, index, nome_membro, mi.st_uid, tam_original, tam_disco, mi.st_mtime, dir->tam_membros, offset, comprimido);
    
    return 0;
}


