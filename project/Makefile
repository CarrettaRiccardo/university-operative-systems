# Comandi gcc
CC = gcc
FLAGS = -std=gnu90

# Cartelle codice sorgente
SRC_DIR = ./src
INCLUDE_DIR = $(SRC_DIR)/include
LIB_DIR = $(SRC_DIR)/lib

# Cartelle output
OBJ_DIR = ./obj
BIN_DIR = ./bin

# File da compilare
LIB = list utils ipc								# Librerie
BIN = bulb fridge window hub timer controller     	# Eseguibili 


# Lista file oggetto librerie. Necessario per linkare le librerie agli eseguibili
OBJ = $(patsubst %,$(OBJ_DIR)/%.o,$(LIB))


#################### RULES ####################
# Stampa una guida sull'utilizzo del programma
help:
	@echo "TODO HELP"


# Macro per compilazione librerie in file .o
$(LIB):
	$(CC) $(FLAGS) -c -o $(OBJ_DIR)/$@.o $(LIB_DIR)/$@.c

# Macro per compilazione eseguibili dispositivi
$(BIN):
	$(CC) $(FLAGS) -o $(BIN_DIR)/$@ $(SRC_DIR)/$@/$@.c $(OBJ)

# Crea le cartelle "bin" e "obj"
prepare:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

# Compila le librerie e gli eseguibili utilizzando le macro
build: prepare $(LIB) $(BIN)


# Rimuove i file compilati
clean:
	@rm -rf $(BIN_DIR)/*
	@rm -rf $(OBJ_DIR)/*
