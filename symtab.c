/*Tabela de Símbolos*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "symtab.h"

#define SIZE 211
#define SHIFT 4

static int hash(char *key) {
    int temp = 0;
    int i = 0;
    while (key[i] != '\0') {
        temp = ((temp << SHIFT) + key[i]) % SIZE;
        ++i;
    }
    return temp;
}

typedef struct LineListRec {
    int lineno;
    struct LineListRec *next;
} *LineList;

typedef struct BucketListRec {
    char *name;
    LineList lines;
    int memloc;
    ExpType type;
    char *scope;
    struct BucketListRec *next;
} *BucketList;

static BucketList hashTable[SIZE];

void st_insert(char *name, int lineno, int loc, ExpType type, char *scope) {
    int h = hash(name);
    BucketList l = hashTable[h];
    
    while ((l != NULL) && (strcmp(name, l->name) != 0)) l = l->next;
    
    if (l == NULL) {
        l = (BucketList)malloc(sizeof(struct BucketListRec));
        l->name = name;
        l->lines = (LineList)malloc(sizeof(struct LineListRec));
        l->lines->lineno = lineno;
        l->memloc = loc;
        l->type = type;
        l->scope = scope;
        l->lines->next = NULL;
        l->next = hashTable[h];
        hashTable[h] = l;
    } else {
        LineList t = l->lines;
        while (t->next != NULL) t = t->next;
        t->next = (LineList)malloc(sizeof(struct LineListRec));
        t->next->lineno = lineno;
        t->next->next = NULL;
    }
}

int st_lookup(char *name) {
    int h = hash(name);
    BucketList l = hashTable[h];
    while ((l != NULL) && (strcmp(name, l->name) != 0)) l = l->next;
    if (l == NULL) return -1;
    else return l->memloc;
}

int st_lookup_scope(char *name, char *scope) {
    int h = hash(name);
    BucketList l = hashTable[h];
    
    while (l != NULL) {
        /* Verifica se o nome E o escopo são iguais */
        if (strcmp(name, l->name) == 0 && strcmp(scope, l->scope) == 0) {
            return l->memloc;  /* Encontrou no mesmo escopo */
        }
        l = l->next;
    }
    
    return -1;  /* Não encontrou no escopo */
}

void printSymTab(FILE *listing) {
    int i;
    
    /* Cabeçalho da tabela - SEM coluna de Linha */
    fprintf(listing, "%-15s %-15s %-10s\n", "Nome", "Escopo", "Tipo");
    fprintf(listing, "%-15s %-15s %-10s\n", "---------------", "---------------", "----------");
    
    for (i = 0; i < SIZE; ++i) {
        if (hashTable[i] != NULL) {
            BucketList l = hashTable[i];
            while (l != NULL) {
                /* Imprime: Nome, Escopo, Tipo - SEM linhas */
                fprintf(listing, "%-15s %-15s ", l->name, l->scope);
                
                switch (l->type) {
                case Integer: fprintf(listing, "%-10s", "int"); break;
                case Void: fprintf(listing, "%-10s", "void"); break;
                case IntegerArray: fprintf(listing, "%-10s", "int[]"); break;
                default: fprintf(listing, "%-10s", "?"); break;
                }
                
                fprintf(listing, "\n");
                l = l->next;
            }
        }
    }
}