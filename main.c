/*
 * Compilador para Linguagem C-
 * Arquivo principal
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"
#include "analyze.h"
#include "symtab.h"
#include "cgen.h"

int lineno = 0;
FILE *source;
FILE *listing;
int EchoSource = FALSE;
int TraceScan = FALSE;
int TraceParse = FALSE;
int TraceAnalyze = FALSE;
int TraceCode = FALSE;
int Error = FALSE;
int SemanticError = FALSE;

int main(int argc, char *argv[]) {
    TreeNode *arvore_sintatica;
    char pgm[120];
    char baseName[120];
    char dotFilename[150];
    char pngFilename[150];
    
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo.cm>\n", argv[0]);
        exit(1);
    }
    
    strcpy(pgm, argv[1]);
    
    /* Extrai o nome base do arquivo (sem extensão e sem caminho) */
    strcpy(baseName, pgm);
    
    /* Remove caminho (se houver) - pega apenas o nome do arquivo */
    char *lastSlash = strrchr(baseName, '\\');
    char *lastFwdSlash = strrchr(baseName, '/');
    char *fileName = baseName;
    
    if (lastSlash != NULL) {
        fileName = lastSlash + 1;
    } else if (lastFwdSlash != NULL) {
        fileName = lastFwdSlash + 1;
    }
    
    /* Move o nome do arquivo para o início de baseName */
    if (fileName != baseName) {
        strcpy(baseName, fileName);
    }
    
    /* Remove a extensão */
    char *dot = strrchr(baseName, '.');
    if (dot != NULL) *dot = '\0';
    
    /* Gera nomes dos arquivos de saída para Graphviz */
    snprintf(dotFilename, sizeof(dotFilename), "ast_%s.dot", baseName);
    snprintf(pngFilename, sizeof(pngFilename), "ast_%s.png", baseName);
    
    source = fopen(pgm, "r");
    if (source == NULL) {
        fprintf(stderr, "Erro: Arquivo %s nao encontrado\n", pgm);
        exit(1);
    }
    
    listing = stdout;
    
    fprintf(listing, "\n========================================\n");
    fprintf(listing, "    COMPILADOR C- - UNIFESP\n");
    fprintf(listing, "========================================\n");
    fprintf(listing, "Arquivo: %s\n", pgm);
    fprintf(listing, "========================================\n\n");
    
    /* FASE 1: Análise Léxica e Sintática */
    fprintf(listing, "=== ANALISE LEXICA E SINTATICA ===\n");
    arvore_sintatica = parse();
    fclose(source);
    
    if (Error) {
        fprintf(listing, "\n========================================\n");
        fprintf(listing, "COMPILACAO ABORTADA: Erros detectados na analise\n");
        fprintf(listing, "========================================\n");
        fprintf(listing, "\nPara recompilar o programa, execute:\n");
        fprintf(listing, "  gcc -o compilador.exe main.c scan.c parse.c analyze.c symtab.c util.c cgen.c\n");
        fprintf(listing, "\nDepois execute novamente:\n");
        fprintf(listing, "  compilador.exe %s\n\n", pgm);
        exit(1);
    }
    fprintf(listing, "OK - Analise lexica e sintatica concluida com sucesso\n\n");
    
    /* FASE 2: Análise Semântica */
    fprintf(listing, "=== ANALISE SEMANTICA ===\n");
    buildSymtab(arvore_sintatica);
    
    if (Error) {
        fprintf(listing, "\n========================================\n");
        fprintf(listing, "COMPILACAO ABORTADA: Erros semanticos detectados\n");
        fprintf(listing, "========================================\n");
        fprintf(listing, "\nCorrija os erros acima e execute novamente:\n");
        fprintf(listing, "  ./cminus.exe.exe %s\n\n", pgm);
        exit(1);
    }
    
    typeCheck(arvore_sintatica);
    
    if (Error) {
        fprintf(listing, "\n========================================\n");
        fprintf(listing, "COMPILACAO ABORTADA: Erros de tipo detectados\n");
        fprintf(listing, "========================================\n");
        fprintf(listing, "\nCorrija os erros acima e execute novamente:\n");
        fprintf(listing, "  ./cminus.exe %s\n\n", pgm);
        exit(1);
    }
    fprintf(listing, "OK - Analise semantica concluida com sucesso\n\n");
    
    /* SAÍDA 1: Tabela de Símbolos */
    fprintf(listing, "========================================\n");
    fprintf(listing, "    TABELA DE SIMBOLOS\n");
    fprintf(listing, "========================================\n");
    printSymTab(listing);
    fprintf(listing, "\n");
    
    /* SAÍDA 2: Árvore Sintática Abstrata (Textual) */
    fprintf(listing, "========================================\n");
    fprintf(listing, "    ARVORE SINTATICA ABSTRATA (AST)\n");
    fprintf(listing, "========================================\n");
    printTreeDot(arvore_sintatica, dotFilename, pngFilename);
    
    /* FASE 3: Geração de Código Intermediário */
    fprintf(listing, "========================================\n");
    fprintf(listing, "    CODIGO INTERMEDIARIO\n");
    fprintf(listing, "========================================\n");
    codeGen(arvore_sintatica);
    fprintf(listing, "\n");

    fprintf(listing, "\n");
    
    fprintf(listing, "========================================\n");
    fprintf(listing, "COMPILACAO CONCLUIDA!\n");
    fprintf(listing, "========================================\n");
    fprintf(listing, "\n");
    
    return 0;
}