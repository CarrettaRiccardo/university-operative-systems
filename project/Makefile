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
LIB = list									# Librerie
BIN = bulb fridge hub timer window		# Eseguibili (tranne "controller" che deve essere compilato con il main)

# Lista file oggetto librerie. Necessario per linkare le libreri agli eseguibili
OBJ = $(patsubst %,$(OBJ_DIR)/%.o,$(LIB))


#################### RULES ####################
# Stampa una guida sull'utilizzo del programma
help:
	@echo "TODO HELP"


# Macro per compilazione librerie in file .o
$(LIB):
	$(CC) $(FLAGS) -c -o $(OBJ_DIR)/$@.o $(LIB_DIR)/$@.c

# Macro per compilazione eseguibili
$(BIN):
	$(CC) $(FLAGS) -o $(BIN_DIR)/$@ $(SRC_DIR)/$@/$@.c $(OBJ)

# Crea le cartelle "bin" e "obj"
prepare:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)


controller:
	$(CC) $(FLAGS) -o $(BIN_DIR)/controller $(SRC_DIR)/controller/main.c $(SRC_DIR)/controller/controller.c


# Compila le librerie e gli eseguibili utilizzando le macro
build: prepare controller $(LIB) $(BIN)


# Rimuove i file compilati
clean:
	@rm -rf $(BIN_DIR)/*
	@rm -rf $(OBJ_DIR)/*
