/*symtab.h - Tabela de Símbolos */

#ifndef SYMTAB_H
#define SYMTAB_H

#include "globals.h"

/* Insere identificador na tabela */
void st_insert(char *name, int lineno, int loc, ExpType type, char *scope);

/* Busca identificador na tabela */
int st_lookup(char *name);

/* Busca identificador em escopo específico */
int st_lookup_scope(char *name, char *scope);

/* Imprime a tabela de símbolos */
void printSymTab(FILE *listing);

#endif