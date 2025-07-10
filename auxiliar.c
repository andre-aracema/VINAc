#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "directory.h"
#include "archive.h"
#include "auxiliar.h"

size_t max_tam_buffer(struct Directory *dir, size_t tam_atual){

    size_t max_tam = tam_atual;
    for(unsigned int i = 0; i < dir->tam_membros; i++){
        if(max_tam < dir->membros[i].tam_disco){
            max_tam = dir->membros[i].tam_disco;
        }
    }

    return max_tam;
}

void fix_member_offset(struct Directory *dir, unsigned int index, unsigned int tam_final, long deslocamento){

    for(unsigned int i = index; i < tam_final; i++){
        dir->membros[i].offset += deslocamento;
    }
}

void create_name(const char *nome_original, char *nome_final, size_t tam_buffer){
    
    char nome_base[256];
    char extensao[256];
    char *ponto = strrchr(nome_original, '.');

    // possui extensão
    if(ponto != NULL){
        size_t base_len = ponto - nome_original;
        strncpy(nome_base, nome_original, base_len);

        nome_base[base_len] = '\0'; 
        strcpy(extensao, ponto);
    }else{
        strcpy(nome_base, nome_original);
        extensao[0] = '\0';
    }

    int cont = 1;

    // criando nome
    snprintf(nome_final, tam_buffer, "%s%s", nome_base, extensao);

    // varificando se nome já exite, se sim muda de nome para nome(n), n = Números Naturais; 
    while(access(nome_final, F_OK) == 0){
        snprintf(nome_final, tam_buffer, "%s(%d)%s", nome_base, cont, extensao);
        cont++;
    }
}

int move_data(FILE *arch, long origem, long deslocamento, size_t tam_dados, size_t buffer_size){

    if(arch == NULL){
        return -1;
    }

    if(tam_dados == 0 || deslocamento == 0){
        return 0;
    }

    unsigned char *buffer = (unsigned char *)malloc(buffer_size);
    if(buffer == NULL){
        return -1;
    }

    size_t bytes_restantes = tam_dados;
    long offset_read;
    long offset_write;

    if(deslocamento > 0){
        offset_read = origem + tam_dados;
        offset_write = offset_read + deslocamento;

        // deslocando os dados no archive para direita
        while(bytes_restantes > 0){
            size_t bloco;
            if(bytes_restantes < buffer_size){
                bloco = bytes_restantes;
            }else{
                bloco = buffer_size;
            }

            offset_read -= (long int)bloco;
            offset_write -= (long int)bloco;

            fseek(arch, offset_read, SEEK_SET);
            fread(buffer, 1, bloco, arch);
            fseek(arch, offset_write, SEEK_SET);
            fwrite(buffer, 1, bloco, arch);

            bytes_restantes -= (long int)bloco;
        }
    }else{
        offset_read = origem;
        offset_write = origem + deslocamento;

        // deslocando os dados no archive para esquerda
        while(bytes_restantes > 0){

            size_t bloco;
            if(bytes_restantes < buffer_size){
                bloco = bytes_restantes;
            }else{
                bloco = buffer_size;
            }

            fseek(arch, offset_read, SEEK_SET);
            fread(buffer, 1, bloco, arch);
            fseek(arch, offset_write, SEEK_SET);
            fwrite(buffer, 1, bloco, arch);

            offset_read += (long int)bloco;
            offset_write += (long int)bloco;
            bytes_restantes -= (long int)bloco;
        }
    }

    free(buffer);
    return 0;
}