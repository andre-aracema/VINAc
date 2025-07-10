#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "archive.h"
#include "directory.h"


int main(int argc, char *argv[]) {

    // mínimo de argumentos passados
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <opção> <archive> [membro1 membro2 ...]\n", argv[0]);
        return 1;
    }

    const char *opcao = argv[1];
    const char *archive_nome = argv[2];
    FILE *archive = NULL;

    switch (opcao[1]) {

        // -p <archive> [membros]: inserir sem compressão
        case 'p': 

        // -i <archive> [membros]: inserir com compressão
        case 'i': 
            archive = fopen(archive_nome, "rb+");
            if(archive == NULL){

                // cria um arquivo
                archive = fopen(archive_nome, "wb+");
                if(archive == NULL){
                    return 1;
                }
            } 

            // inserindo os membros
            for(int i = 3; i < argc; i++){
                if(opcao[1] == 'p'){
                    if(insert_member(archive, argv[i]) != 0){
                        fprintf(stderr, "Erro ao tentar inserir sem compressão\n");
                        return 1;
                    }
                }else{
                    if(insert_member_compressed(archive, argv[i]) != 0){
                        fprintf(stderr, "Erro ao tentar inserir com compressão\n");
                        return 1;
                    }
                }
            }

            fclose(archive);
            break;

        // -m <archive> <membro> <membro_target>: move <membro> para imediatamente depois do <membro_target>
        case 'm': 

            // mínimo de argumetos para a função -m
            if((argc != 4) && (argc != 5)){
                fprintf(stderr, "Uso: %s -m <archive> <membro> <membro_target>\n", argv[0]);
                return 1;
            }

            archive = fopen(archive_nome, "rb+");
            if(archive == NULL){
                perror("Aquivo não existe");
                return 1;
            }

            // <archive> <membro> <membro_target>, respectivamente
            if(move_member(archive, argv[3], argv[4]) != 0){
                fprintf(stderr, "Erro ao tentar mover\n");
                return 1;
            } 

            fclose(archive);
            break;

        // -x <archive> <membro>: extrair membros ou -x <archive>: extrair todos os membros
        case 'x': 
            archive = fopen(archive_nome, "rb");
            if(archive == NULL){
                perror("Aquivo não existe");
                return 1;
            }

            // extrair todos os membros
            if(argc == 3){
                struct Directory dir;

                // itera sobre todos os membros e os extrai
                if(read_directory(archive, &dir) != 0){
                    fprintf(stderr, "Erro ao tentar ler o diretório\n");
                    return 1;
                }
                for(unsigned int i = 0; i < dir.tam_membros; i++){
                    if(extract_members(archive, dir.membros[i].nome) != 0){
                        fprintf(stderr, "Erro ao tentar extrair\n");
                        return 1;
                    }
                }
                
                free(dir.membros);
            }else{

                // extrair membros específicos
                for(int i = 3; i < argc; i++){
                    if(extract_members(archive, argv[i]) != 0){
                        fprintf(stderr, "Erro ao tentar extrair\n");
                        return 1;
                    }

                    fseek(archive, 0, SEEK_SET); 
                }
            }

            fclose(archive);
            break;

        // -r <archive> [membros]: remover membros
        case 'r': 
            archive = fopen(archive_nome, "rb+");
            if(archive == NULL){
                perror("Aquivo não existe");
                return 1;
            }

            for(int i = 3; i < argc; i++){
                fseek(archive, 0, SEEK_SET);
                if(remove_member(archive, argv[i]) != 0){
                    fprintf(stderr, "Erro ao tentar remover\n");
                    return 1;
                }
            }
            
            fclose(archive);
            break;

        // -c <archive>: listar conteúdo
        case 'c': 
            struct Directory dir;

            archive = fopen(archive_nome, "rb");
            if(archive == NULL){
                perror("Aquivo não existe");
                return 1;
            }

            if(read_directory(archive, &dir) != 0){
                fprintf(stderr, "Erro ao tentar ler o diretório\n");
                return 1;
            }

            if(list_members(&dir) != 0){
                fprintf(stderr, "Archive vazio\n");
                return 1;
            }

            fclose(archive);
            break;
        
        // opções erradas
        default:
            fprintf(stderr, "Opção inválida: %s\n", opcao);
            return 1;
    }

    return 0;
}
