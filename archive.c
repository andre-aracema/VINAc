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
#include "lz.h"

int list_members(struct Directory *dir){

    if(dir == NULL || dir->membros == NULL || dir->tam_membros == 0){
        return -1;
    }

    printf("=== Conteúdo do Archive ===\n\n");

    for(unsigned int i = 0; i < dir->tam_membros; i++){
        struct MemberInfo *member = &dir->membros[i];

        // lidando com o horário
        char mod_data_str[26];
        strncpy(mod_data_str, ctime(&member->mod_data), sizeof(mod_data_str));
        mod_data_str[strcspn(mod_data_str, "\n")] = '\0';

        printf("--------------------------------------------------------------------------------------------------------\n");
        printf("Membro #%u\n", member->ordem);
        printf("  Nome:                %s\n", member->nome);
        printf("  UID:                 %u\n", member->uid);
        printf("  Tamanho Original:    %zu bytes\n", member->tam_original);
        printf("  Tamanho em Disco:    %zu bytes\n", member->tam_disco);
        printf("  Data de Modificação: %s\n", mod_data_str);
        printf("  Offset:              %zu\n", member->offset);
        printf("\n");
    }

    printf("--------------------------------------------------------------------------------------------------------\n");

    free(dir->membros);
    return 0;
}

int insert_member(FILE *arch, const char *nome_membro){

    if(arch == NULL){
        return -1;
    }

    FILE *arquivo_membro = fopen(nome_membro, "rb");
    if(arquivo_membro == NULL){
        return -1;
    }

    struct Directory dir;
    size_t tam_dados_move;
    size_t offset_dados;
    long origem;
    long deslocamento = 0;
    int index_rep;

    fseek(arquivo_membro, 0, SEEK_END);
    size_t size_membro = ftell(arquivo_membro);

    fseek(arch, 0, SEEK_END);
    size_t tam_arquivo = ftell(arch);

    // archive zerado, inicializando archive
    if (tam_arquivo == 0) {
        dir.tam_membros = 0;
        dir.offset_fim_diretorio = OFFSET_START_DIRECTORY;
        dir.membros = NULL;

        write_directory(arch, &dir);

        // atualizando tamanho do archive
        fseek(arch, 0, SEEK_END);
        tam_arquivo = ftell(arch);
    }

    read_directory(arch, &dir);

    size_t tam_buffer = max_tam_buffer(&dir, size_membro);

    // caso 1: archive sem membros
    if(dir.tam_membros == 0){
        offset_dados = dir.offset_fim_diretorio + sizeof(struct MemberInfo);
        insert_member_directory(&dir, nome_membro, size_membro, size_membro, offset_dados, 0);
        write_directory(arch, &dir);
    }
    
    // caso 2: sobrescrita do membro (atualização)
    else if((index_rep = find_member_directory(&dir, nome_membro)) != -1){
        size_t tam_antigo = dir.membros[index_rep].tam_disco;
        deslocamento = size_membro - tam_antigo;
        origem = dir.membros[index_rep].offset + tam_antigo;

        fseek(arch, 0, SEEK_END);
        size_t tam_aux = ftell(arch);

        // membro a ser atualiza tem necessidade de mover dados
        if((origem != (long)tam_aux) && (deslocamento != 0)){
            tam_dados_move = tam_arquivo - origem;
            move_data(arch, origem, deslocamento, tam_dados_move, tam_buffer);
            fix_member_offset(&dir, index_rep + 1, dir.tam_membros, deslocamento);
        }

        offset_dados = dir.membros[index_rep].offset;

        struct stat mi;
        if(stat(nome_membro, &mi) != 0){
            free(dir.membros);
            fclose(arquivo_membro);
            return -1;
        }

        // atualizando metadados
        initialize_member_directory(&dir, index_rep, nome_membro, mi.st_uid, size_membro, size_membro, mi.st_mtime, dir.membros[index_rep].ordem, offset_dados, 0);
        write_directory(arch, &dir);
    }

    // caso 3: inserindo novo membro ao final do archive
    else{
        origem = dir.offset_fim_diretorio;
        tam_dados_move = tam_arquivo - origem;

        move_data(arch, origem, sizeof(struct MemberInfo), tam_dados_move, tam_buffer);
        fix_member_offset(&dir, 0, dir.tam_membros, sizeof(struct MemberInfo));

        if(tam_dados_move == 0){
            fseek(arch, origem + sizeof(struct MemberInfo), SEEK_SET);
            offset_dados = ftell(arch);
        }
        else{
            fseek(arch, 0, SEEK_END);
            offset_dados = ftell(arch);
        }

        insert_member_directory(&dir, nome_membro, size_membro, size_membro, offset_dados, 0);     
        write_directory(arch, &dir);   
    }

    // escrevendo os novos dados
    unsigned char *buffer = (unsigned char *)malloc(tam_buffer);
    if(buffer == NULL){
        free(dir.membros);
        fclose(arquivo_membro);
        return -1;
    }

    fseek(arquivo_membro, 0, SEEK_SET);
    fread(buffer, 1, size_membro, arquivo_membro);
    fseek(arch, offset_dados, SEEK_SET);
    fwrite(buffer, 1, size_membro, arch);

    if((deslocamento != 0) && (index_rep != -1)){
        ftruncate(fileno(arch), tam_arquivo + deslocamento);
    }
    else if ((deslocamento == 0) && (index_rep != -1)){
       // o tamanho do archive não se altera
    }else{
        ftruncate(fileno(arch), tam_arquivo + sizeof(struct MemberInfo) + size_membro);
    }

    free(buffer);
    free(dir.membros);
    fclose(arquivo_membro);
    return 0;
}

int insert_member_compressed(FILE *arch, const char *nome_membro){

    if(arch == NULL){
        return -1;
    }

    FILE *arquivo_membro = fopen(nome_membro, "rb");
    if(arquivo_membro == NULL){
        return -1;
    }

    struct Directory dir;
    size_t tam_armazenado;
    size_t offset_dados;
    size_t tam_dados_move;
    long origem;
    long deslocamento = 0;
    int index_rep;
    int flag_comprimido;
    

    fseek(arquivo_membro, 0, SEEK_END);
    size_t size_membro = ftell(arquivo_membro);

    fseek(arch, 0, SEEK_END);
    size_t tam_arquivo = ftell(arch);

    // archive zerado
    if (tam_arquivo == 0) {
        dir.tam_membros = 0;
        dir.offset_fim_diretorio = OFFSET_START_DIRECTORY;
        dir.membros = NULL;

        write_directory(arch, &dir);

        fseek(arch, 0, SEEK_END);
        tam_arquivo = ftell(arch);
    }

    read_directory(arch, &dir);

    size_t tam_buffer = max_tam_buffer(&dir, size_membro);

    // buffer utilzado para salvar os dados originais
    unsigned char *buffer_in = (unsigned char *)malloc(tam_buffer);
    if(buffer_in == NULL){
        fclose(arquivo_membro);
        return -1;
    }

    // buffer utilizado para salvar os dados compactados
    unsigned char *buffer_out = (unsigned char *)malloc(tam_buffer*2); // por segurança, visto que não sera utilizado para nada além da compressão
    if(buffer_in == NULL){
        fclose(arquivo_membro);
        free(buffer_in);
        return -1;
    }

    fseek(arquivo_membro, 0, SEEK_SET);
    fread(buffer_in, 1, tam_buffer, arquivo_membro);   

    // comprimindo
    int size_compress = LZ_Compress(buffer_in, buffer_out, size_membro);

    // caso 1: archive sem membros
    if(dir.tam_membros == 0){
        offset_dados = dir.offset_fim_diretorio + sizeof(struct MemberInfo);

        // se membro comprimido tiver mais dados, utilizamos o tamanho original (descomprimido) 
        if((size_t)size_compress < size_membro){
            tam_armazenado = size_compress;
            flag_comprimido = 1;
        }else{
            tam_armazenado = size_membro;
            flag_comprimido = 0;
        }
            
        insert_member_directory(&dir, nome_membro, size_membro, tam_armazenado, offset_dados, flag_comprimido);
        write_directory(arch, &dir);
    }
    
    // caso 2: sobrescrita do membro (atualização)
    else if((index_rep = find_member_directory(&dir, nome_membro)) != -1){
        struct stat mi;
        if(stat(nome_membro, &mi) != 0){
            fclose(arquivo_membro);
            free(buffer_in);
            free(buffer_out);
            free(dir.membros);
            return -1;
        }

        offset_dados = dir.membros[index_rep].offset;

        // se membro comprimido tiver mais dados, utilizamos o tamanho original (descomprimido) 
        if((size_t)size_compress < size_membro){
            tam_armazenado = size_compress;
            flag_comprimido = 1;
        }else{
            tam_armazenado = size_membro;
            flag_comprimido = 0;
        }

        size_t tam_antigo = dir.membros[index_rep].tam_disco;
        deslocamento = tam_armazenado - tam_antigo;
        origem = dir.membros[index_rep].offset + tam_antigo;

        // membro a ser atualiza tem necessidade de mover dados
        if((origem != (long)tam_arquivo) && (deslocamento != 0)){
            tam_dados_move = tam_arquivo - origem;
            move_data(arch, origem, deslocamento, tam_dados_move, tam_armazenado);
            fix_member_offset(&dir, index_rep + 1, dir.tam_membros, deslocamento);
        }
        
        initialize_member_directory(&dir, index_rep, nome_membro, mi.st_uid, size_membro, tam_armazenado, mi.st_mtime, dir.membros[index_rep].ordem, offset_dados, flag_comprimido);
        write_directory(arch, &dir);
    }

    // caso 3: inserindo novo membro ao final do archive
    else{
        origem = dir.offset_fim_diretorio;
        tam_dados_move = tam_arquivo - origem;

        move_data(arch, origem, sizeof(struct MemberInfo), tam_dados_move, tam_buffer);
        fix_member_offset(&dir, 0, dir.tam_membros, sizeof(struct MemberInfo));

        if(tam_dados_move == 0){
            fseek(arch, origem + sizeof(struct MemberInfo), SEEK_SET);
            offset_dados = ftell(arch);
        }
        else{
            fseek(arch, 0, SEEK_END);
            offset_dados = ftell(arch);
        }

        // se membro comprimido tiver mais dados, utilizamos o tamanho original (descomprimido) 
        if((size_t)size_compress < size_membro){
            tam_armazenado = size_compress;
            flag_comprimido = 1;
        }else{
            tam_armazenado = size_membro;
            flag_comprimido = 0;
        }

        insert_member_directory(&dir, nome_membro, size_membro, tam_armazenado, offset_dados, flag_comprimido);
        write_directory(arch, &dir);
    }

    // lidando com a escrita
    unsigned char *buffer_escrita;
    
    if((size_t)size_compress < size_membro){
        buffer_escrita = buffer_out;
        tam_armazenado = size_compress;
    }else{
        buffer_escrita = buffer_in;
        tam_armazenado = size_membro;
    }

    fseek(arch, offset_dados, SEEK_SET);
    fwrite(buffer_escrita, 1, tam_armazenado, arch);

    // ajustando tamanho
    if(index_rep != -1){
        if(deslocamento != 0){
            ftruncate(fileno(arch), tam_arquivo + deslocamento);
        }
    }else{
        ftruncate(fileno(arch), tam_arquivo + tam_armazenado + sizeof(struct MemberInfo));
    }

    free(buffer_in);
    free(buffer_out);
    free(dir.membros);
    fclose(arquivo_membro);
    return 0;
}

int remove_member(FILE *arch, const char *nome_membro){

    if(arch == NULL){
        return -1;
    }

    struct Directory dir;

    read_directory(arch, &dir);

    int index = find_member_directory(&dir, nome_membro);
    if(index == -1){
        free(dir.membros);
        return -1;
    }

    size_t offset_remove = dir.membros[index].offset;
    size_t tam_remove = dir.membros[index].tam_disco;
    size_t origem = offset_remove + tam_remove;
    
    fseek(arch, 0, SEEK_END);
    size_t tam_total = ftell(arch);

    size_t tam_buffer = max_tam_buffer(&dir, tam_remove);

    size_t tam_move_dados = tam_total - origem;
    if(tam_move_dados > 0){
        move_data(arch, origem, -tam_remove, tam_move_dados, tam_buffer);
        fix_member_offset(&dir, index + 1, dir.tam_membros, -tam_remove);
    }    
    
    remove_member_directory(&dir, nome_membro);
    move_data(arch, dir.offset_fim_diretorio, -sizeof(struct MemberInfo), tam_total - dir.offset_fim_diretorio, tam_buffer);
    fix_member_offset(&dir, 0, dir.tam_membros, -sizeof(struct MemberInfo));
    write_directory(arch, &dir);

    if(dir.tam_membros == 0){
        ftruncate(fileno(arch), dir.offset_fim_diretorio);
    }else{
        ftruncate(fileno(arch), tam_total - tam_remove - sizeof(struct MemberInfo)); 
    }

    free(dir.membros);
    return 0;
}

int extract_members(FILE *arch, const char *nome_membro){

    if(arch == NULL){
        return -1;
    }

    struct Directory dir;

    read_directory(arch, &dir);

    int index = find_member_directory(&dir, nome_membro);
    if(index == -1){
        free(dir.membros);
        return -1;
    }

    size_t tam_membro = dir.membros[index].tam_original;
    size_t tam_membro_comprimido = dir.membros[index].tam_disco;
    
    unsigned char *buffer = (unsigned char *)malloc(tam_membro);
    if(buffer == NULL){
        free(dir.membros);
        return -1;
    }

    unsigned char *buffer_comprimido = (unsigned char *)malloc(tam_membro);
    if(buffer_comprimido == NULL){
        free(dir.membros);
        free(buffer);
        return -1;
    }

    fseek(arch, dir.membros[index].offset, SEEK_SET);

    // desconpactando (se necessário)
    if(dir.membros[index].comprimido == 1){
        fread(buffer_comprimido, 1, tam_membro_comprimido, arch);
        LZ_Uncompress(buffer_comprimido, buffer, tam_membro_comprimido);
    }else{
        fread(buffer, 1, tam_membro, arch);
    }
    
    // verificando se o arquivo exite (mudança de nome)
    FILE *new_arquivo = NULL;
    if(access(nome_membro, F_OK) == 0){
        // extrai com nome diferente
        char nome_membro_mod[MAX_LEN_NAME];
        create_name(nome_membro, nome_membro_mod, MAX_LEN_NAME);

        new_arquivo = fopen(nome_membro_mod, "wb+");
        if(new_arquivo == NULL){
            free(buffer);
            free(buffer_comprimido);
            free(dir.membros);
            return -1;
        }
    }else{
        // extrai com o mesmo nome
        new_arquivo = fopen(nome_membro, "wb+");
        if(new_arquivo == NULL){
            free(buffer);
            free(buffer_comprimido);
            free(dir.membros);
            return -1;
        }
    }

    fseek(new_arquivo, 0, SEEK_SET);
    fwrite(buffer, 1, tam_membro, new_arquivo);
    fclose(new_arquivo);

    free(buffer);
    free(buffer_comprimido);
    free(dir.membros);
    return 0;
}

int move_member(FILE *arch, const char *nome_membro, const char *nome_membro_target){
    
    if(arch == NULL){
        return -1;
    }

    struct Directory dir;
    
    read_directory(arch, &dir);

    int index_membro = find_member_directory(&dir, nome_membro);
    if(index_membro == -1){
        free(dir.membros);
        return -1;
    }

    int index_membro_target;
    int move_inicio = 0;

    if(nome_membro_target != NULL){
        index_membro_target = find_member_directory(&dir, nome_membro_target);
        if(index_membro_target == -1){
            free(dir.membros);
            return -1;
        }
    }
    else{
        index_membro_target = 0;
        move_inicio = 1;
    }
    
    if (((index_membro == index_membro_target + 1) && (move_inicio == 0)) || (index_membro == index_membro_target)) {
        free(dir.membros);
        return 0; 
    }

    fseek(arch, 0, SEEK_END);
    size_t tam_arquivo = ftell(arch);
    
    size_t tamanho_membro = dir.membros[index_membro].tam_disco;
    long origem = dir.membros[index_membro].offset + tamanho_membro;
    size_t tam_move = tam_arquivo - origem;

    size_t tam_buffer = max_tam_buffer(&dir, tamanho_membro);

    unsigned char *buffer = (unsigned char *)malloc(tam_buffer);
    if(buffer == NULL){
        free(dir.membros);
        return -1;
    }

    fseek(arch, dir.membros[index_membro].offset, SEEK_SET);
    fread(buffer, 1, tam_buffer, arch);

    // reorganizando a posição dos dados
    // estamos sobrescrevendo o membro
    move_data(arch, origem, - tamanho_membro, tam_move, tam_buffer);
    fix_member_offset(&dir, index_membro + 1, dir.tam_membros, -tamanho_membro);

    if(move_inicio){
        size_t tam_dados = tam_arquivo - dir.offset_fim_diretorio;

        move_data(arch, dir.offset_fim_diretorio, tamanho_membro, tam_dados, tam_buffer);
        fix_member_offset(&dir, 0, dir.tam_membros, tamanho_membro);

        fseek(arch, dir.offset_fim_diretorio, SEEK_SET);
        fwrite(buffer, 1, tam_buffer, arch);

        dir.membros[index_membro].offset = dir.offset_fim_diretorio;
        move_member_directory(&dir, index_membro, -1);
    }
    else{
        size_t tamanho_membro_target = dir.membros[index_membro_target].tam_disco;
        long origem_membro_target = dir.membros[index_membro_target].offset + tamanho_membro_target;
        size_t tam_move_target = tam_arquivo - origem_membro_target;

        // estamos abrindo espaço para escrever o membro pós membro_target
        move_data(arch, origem_membro_target, tamanho_membro, tam_move_target, tam_buffer);
        fix_member_offset(&dir, index_membro_target + 1, dir.tam_membros, tamanho_membro);

        origem_membro_target = dir.membros[index_membro_target].offset + tamanho_membro_target;

        // escrevendo o membro no local correto
        fseek(arch, origem_membro_target, SEEK_SET);  
        fwrite(buffer, 1, tam_buffer, arch);

        // arrumando offset do membro movido
        dir.membros[index_membro].offset = dir.membros[index_membro_target].offset + tamanho_membro_target;

        //arrumando diretório
        move_member_directory(&dir, index_membro, index_membro_target);
    }
    
    write_directory(arch, &dir);
    ftruncate(fileno(arch), tam_arquivo);

    free(dir.membros);
    free(buffer);
    return 0;
}