
/*util.h - Funções Utilitárias*/

#ifndef UTIL_H
#define UTIL_H

#include "globals.h"

/* Imprime token */
void printToken(TokenType token, const char *tokenString);

/* Cria novo nó de comando */
TreeNode *newStmtNode(StmtKind kind);

/* Cria novo nó de expressão */
TreeNode *newExpNode(ExpKind kind);

/* Copia string alocando memória */
char *copyString(char *s);

/* Imprime árvore em formato DOT */
void printTreeDot(TreeNode *tree, const char *dotFilename, const char *pngFilename);

#endif