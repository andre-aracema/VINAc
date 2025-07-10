CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -std=c99 -D_POSIX_C_SOURCE=200112L
LDLIBS = -lm

# Nome do executável e arquivos
EXEC = vina
SRC = main.c archive.c auxiliar.c directory.c lz.c 
OBJ = $(SRC:.c=.o)
HEADERS = archive.h auxiliar.h directory.h lz.h

# Regra principal
all: $(EXEC)

# Geração do executável
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Compilação individual de .c em .o
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

# Limpeza dos arquivos gerados
clean:
	rm -f *.o
	rm -f $(EXEC)
