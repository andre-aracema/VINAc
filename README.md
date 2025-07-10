# VINAc - Arquivador com Compressão

VINAc é um programa arquivador com suporte a compressão, ou seja, permite armazenar e organizar arquivos com ou sem compressão. O programa suporta inserção, remoção, extração, listagem e movimentação de arquivos dentro de um archive '.vc'.

## Descrição das Operações

O programa deve ser executado da seguinte maneira:

./vina <opção> <archive> [membros]

Onde a opção deve ser:

'-p' : Insere/acrescenta um ou mais membros sem compressão no archive.

'-i' : Insere/acrescenta um ou mais membros com compressão no archive.

'-m' : Move o membro indicado para imediatamente depois do membro target existente no archive. Sua estrutura deve ser executada da seguinte forma:
'-m <archive> <membro> <membro_target>'

'-x' : Extrai os membros indicados do archive. Se não forem indicados, todos devem ser extraídos.

'-r' : Remove os membros indicados do archive.

'-c' : Lista o conteúdo do archive em ordem, incluindo as propriedades de cada membro.


## Estrutura do Projeto

- main.c : Responsável por orientar qual operação esta sendo executada e abrir/criar o archive.
- archive.c/.h : Função principal, contêm as funções responsáveis pela manipulação em disco.
- directory.c/.h : Manipulação da área do diretório, definição das structs (metadados dos membros).
- auxiliar.c/.h : Funções auxiliares diversas.
- lz.c/.h : Compressão e descompressão LZ.
- makefile : Regras de compilação.


## Algoritmos, Estruturas e Decisões de Projeto

### Estruturas Utilizadas:

- struct Directory : Armazena os metadados de todos os membros em um array dinâmico de struct MemberInfo. Também guarda o offset do final do diretório e a quantidade de membros presentes no diretório. 
- struct MemberInfo : contém nome, uid, tamanho original, tamanho em disco, data de modificação, ordem, offset e uma flag para saber se o membro esta comprimido ou não.


### Principais Algoritmos:

Os seguintes principais algoritmos foram implementados como parte central do programa:

- 'read_directory'/'write_directory' : leem e gravam o diretório no início do archive.

- 'insert_member'/'insert_member_compressed' : adicionam membros com ou sem compressão no archive, fazem substituição quando necessário.

- 'remove_member'/'remove_member_directory' : removem membros do archive e reorgarniza dados do diretório.

- 'move_data' : permitam a movimentação de dados no archive.

- 'extract_members' : extrai membro do archive, descomprimido se necessário.

- 'fix_member_offset'/'move_data' : realocam dados fisicamente no archive e atualizam os offset se necessário.

- 'initialize_member_directory'/'insert_member_directory' : inicializam ou adicionam metadados ao diretório;

- 'list_members' : exibe informação dos membros presentes.

- O diretório é manipulado como um vetor, sendo de fácil modificação, mas podendo ser custoso na inserção e remoção de dados.

- Compressão utilizada : LZ.

- A movimentação de dados é feita por leitura e reescrita dos dados a partir dos offsets originais.


### Alternativas Consideradas:

- Manipulação dos dados do diretório através de um vetor estático: Tamanho limitado, desperdício de memória, transbordamento de memória. Resolvi implementar com um vetor dinâmico, facilitando o ajuste de tamanho e na economia de memória.

- Funções de inserção sem e com compressão juntas : Dificuldade de leitura e entendimento do código. Resolvi separá-las para melhor visualização.

### Dificuldades Encontradas:

- Implementação da função auxiliar 'move_data' : é uma das principais funções do projeto. Tive dificuldade em implementá-la, tendo em vista que foi a primeira função em que comecei o projeto, entretanto, a construção desse algoritmo me permitiu entender o funcionamento do projeto, facilitando na implementação das demais funções.

- Sobrescrita dos dados do diretório : na implementação dos algoritmos de inserção, os offsets dos membros não estavam sendo corretamente atualizadas. Resolvi com o desenvolvimento de uma função auxiliar ('fix_member_offset').


## Autor:
André Akira A Abe Aracema <br>
Estudante de Ciência da Computação - UFPR <br>
Data: 11/05/2025

