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
LIB = list utils ipc							# Librerie
DEVICES = controller bulb fridge window hub timer alarm   # Eseguibili dispositivi


# Lista file oggetto librerie. Necessario per linkare le librerie agli eseguibili
OBJ = $(patsubst %,$(OBJ_DIR)/%.o,$(LIB))


#################### RULES ####################
# Stampa una guida sull'utilizzo del programma
help:
	@echo Per compilare eseguire "make build"
	@echo Per eliminare i file generati dalla compilazione eseguire "make clean"
	@echo Per eseguire il progetto, spostarsi nella cartella "bin" ed eseguire il file "terminal"
	@echo Per eseguire dei comandi manuali, spostarsi nella cartella "bin" ed eseguire il file "manual"

# Compilazione terminal e manual
terminal_manual:
	$(CC) $(FLAGS) -o $(BIN_DIR)/terminal $(SRC_DIR)/terminal/terminal.c $(OBJ)
	$(CC) $(FLAGS) -o $(BIN_DIR)/manual $(SRC_DIR)/terminal/terminal.c -DMANUAL $(OBJ)

# Macro per compilazione librerie in file .o
$(LIB):
	$(CC) $(FLAGS) -c -o $(OBJ_DIR)/$@.o $(LIB_DIR)/$@.c

# Macro per compilazione eseguibili dispositivi
$(DEVICES):
	$(CC) $(FLAGS) -o $(BIN_DIR)/$@ $(SRC_DIR)/$@/$@.c $(OBJ)

# Crea le cartelle "bin" e "obj"
prepare:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

# Compila le librerie e gli eseguibili utilizzando le macro
build: prepare $(LIB) terminal_manual $(DEVICES)


# Rimuove i file compilati
clean:
	@rm -rf $(BIN_DIR)/*
	@rm -rf $(OBJ_DIR)/*
