/*analyze.h - Análise Semântica*/

#ifndef ANALYZE_H
#define ANALYZE_H

#include "globals.h"

/* Constrói a tabela de símbolos */
void buildSymtab(TreeNode *syntaxTree);

/* Verifica tipos na árvore */
void typeCheck(TreeNode *syntaxTree);

#endif