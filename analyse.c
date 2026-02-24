/*Analisador Semântico para C-*/

#include "globals.h"
#include "symtab.h"
#include "util.h"
#include "analyze.h"

#include <stdlib.h>
#include <string.h>

static char *currentScope = "global";
static int location = 0;
static int hasMain = 0;  /* Flag para verificar se main existe */

/* Lista temporária para rastrear símbolos declarados e metadados */
typedef struct SymbolRec {
    char *name;
    char *scope;
    ExpType type;

    int isFunction;     /* 1 se for função, 0 caso contrário */
    int paramCount;     /* quantidade de parâmetros se isFunction=1 */

    struct SymbolRec *next;
} SymbolRec;

static SymbolRec *symbolList = NULL;

/* Adiciona símbolo à lista */
static void addSymbolEx(char *name, char *scope, ExpType type, int isFunction, int paramCount) {
    SymbolRec *rec = (SymbolRec *)malloc(sizeof(SymbolRec));
    rec->name = copyString(name);
    rec->scope = copyString(scope);
    rec->type = type;
    rec->isFunction = isFunction;
    rec->paramCount = paramCount;
    rec->next = symbolList;
    symbolList = rec;
}

static void addSymbol(char *name, char *scope, ExpType type) {
    addSymbolEx(name, scope, type, 0, 0);
}

static SymbolRec* findSymbolRecInScope(const char *name, const char *scope) {
    SymbolRec *rec = symbolList;
    while (rec != NULL) {
        if (strcmp(rec->name, name) == 0 && strcmp(rec->scope, scope) == 0)
            return rec;
        rec = rec->next;
    }
    return NULL;
}

static SymbolRec* findSymbolRecVisible(const char *name, const char *scope) {
    /* procura no escopo atual, senão no global */
    SymbolRec *rec = symbolList;
    while (rec != NULL) {
        if (strcmp(rec->name, name) == 0) {
            if (strcmp(rec->scope, scope) == 0 || strcmp(rec->scope, "global") == 0)
                return rec;
        }
        rec = rec->next;
    }
    return NULL;
}

static int symbolExistsInScope(char *name, char *scope) {
    return findSymbolRecInScope(name, scope) != NULL;
}

/* ===== Helpers para contagem de parâmetros/argumentos ===== */

static int countParams(TreeNode *paramsNode) {
    int count = 0;
    TreeNode *p = paramsNode;

    if (p == NULL) return 0;

    /* caso típico de main(void): um ParamK "vazio" */
    if (p->nodekind == StmtK && p->kind.stmt == ParamK) {
        if (p->attr.name == NULL && p->type == Void) {
            return 0;
        }
    }

    while (p != NULL) {
        if (p->nodekind == StmtK && p->kind.stmt == ParamK) {
            if (p->attr.name != NULL) count++;
        }
        p = p->sibling;
    }
    return count;
}

static int countArgs(TreeNode *argsNode) {
    int count = 0;
    TreeNode *a = argsNode;
    while (a != NULL) {
        count++;
        a = a->sibling;
    }
    return count;
}

/* Inserir funções built-in (input e output) */
static void insertBuiltins(void) {
    /* input(): retorna int, 0 params */
    st_insert("input", 0, location++, Integer, "global");
    addSymbolEx("input", "global", Integer, 1, 0);

    /* output(x): retorna void, 1 param */
    st_insert("output", 0, location++, Void, "global");
    addSymbolEx("output", "global", Void, 1, 1);
}

/* Percorre a árvore em pré-ordem inserindo identificadores na tabela */
static void traverse(TreeNode *t, void (*preProc)(TreeNode *),
                    void (*postProc)(TreeNode *)) {
    if (t != NULL) {
        preProc(t);
        for (int i = 0; i < MAXCHILDREN; i++)
            traverse(t->child[i], preProc, postProc);
        postProc(t);
        traverse(t->sibling, preProc, postProc);
    }
}

/* Pós-processamento para resetar escopo */
static void afterNode(TreeNode *t) {
    if (t != NULL && t->nodekind == StmtK && t->kind.stmt == FunDeclK) {
        currentScope = "global";
    }
}

/* preProc para typeCheck (entra no escopo da função) */
static void enterScopeForTypeCheck(TreeNode *t) {
    if (t != NULL && t->nodekind == StmtK && t->kind.stmt == FunDeclK) {
        currentScope = t->attr.name;
    }
}

/* Insere identificadores na tabela de símbolos */
static void insertNode(TreeNode *t) {
    switch (t->nodekind) {
    case StmtK:
        switch (t->kind.stmt) {

        case FunDeclK: {
            if (symbolExistsInScope(t->attr.name, "global")) {
                fprintf(listing,
                        "\nERRO SEMANTICO: funcao '%s' redeclarada - LINHA: %d\n",
                        t->attr.name, t->lineno);
                Error = TRUE;
                return;
            }

            int nParams = countParams(t->child[0]);

            if (strcmp(t->attr.name, "main") == 0) {
                hasMain = 1;

                if (t->type != Void) {
                    fprintf(listing,
                            "\nERRO SEMANTICO: funcao 'main' deve retornar 'void', nao '%s' - LINHA: %d\n",
                            t->type == Integer ? "int" : "outro tipo",
                            t->lineno);
                    Error = TRUE;
                }

                if (nParams != 0) {
                    fprintf(listing,
                            "\nERRO SEMANTICO: funcao 'main' deve ser 'main(void)' (0 parametros), nao %d - LINHA: %d\n",
                            nParams, t->lineno);
                    Error = TRUE;
                }
            }

            st_insert(t->attr.name, t->lineno, location++, t->type, "global");
            addSymbolEx(t->attr.name, "global", t->type, 1, nParams);

            currentScope = t->attr.name;
            break;
        }

        case VarDeclK:
            if (symbolExistsInScope(t->attr.name, currentScope)) {
                fprintf(listing,
                        "\nERRO SEMANTICO: variavel '%s' ja declarada no escopo '%s' - LINHA: %d\n",
                        t->attr.name, currentScope, t->lineno);
                Error = TRUE;
                return;
            }
            st_insert(t->attr.name, t->lineno, location++, t->type, currentScope);
            addSymbol(t->attr.name, currentScope, t->type);
            break;

        case ParamK:
            if (t->attr.name != NULL) {
                if (symbolExistsInScope(t->attr.name, currentScope)) {
                    fprintf(listing,
                            "\nERRO SEMANTICO: parametro '%s' ja declarado na funcao '%s' - LINHA: %d\n",
                            t->attr.name, currentScope, t->lineno);
                    Error = TRUE;
                    return;
                }
                st_insert(t->attr.name, t->lineno, location++, t->type, currentScope);
                addSymbol(t->attr.name, currentScope, t->type);
            }
            break;

        case AssignK: {
            SymbolRec *rec = findSymbolRecVisible(t->attr.name, currentScope);

            if (rec == NULL) {
                fprintf(listing,
                        "\nERRO SEMANTICO: identificador '%s' nao declarado - LINHA: %d\n",
                        t->attr.name, t->lineno);
                Error = TRUE;
            }
            /* (1) não pode atribuir em função */
            else if (rec->isFunction) {
                fprintf(listing,
                        "\nERRO SEMANTICO: atribuicao a funcao '%s' - LINHA: %d\n",
                        t->attr.name, t->lineno);
                Error = TRUE;
            }
            /* (2) não pode atribuir em array sem indice: v = 5; */
            else if (rec->type == IntegerArray) {
                fprintf(listing,
                        "\nERRO SEMANTICO: atribuicao de indice ao array '%s' (use '%s[i] = ...') - LINHA: %d\n",
                        t->attr.name, t->attr.name, t->lineno);
                Error = TRUE;
            }
            break;
        }

        default:
            break;
        }
        break;

    case ExpK:
        switch (t->kind.exp) {
        case IdK:
        case ArrIdK: {
            SymbolRec *rec = findSymbolRecVisible(t->attr.name, currentScope);
            if (rec == NULL) {
                fprintf(listing,
                        "\nERRO SEMANTICO: identificador '%s' nao declarado - LINHA: %d\n",
                        t->attr.name, t->lineno);
                Error = TRUE;
            } else {
                st_insert(t->attr.name, t->lineno, 0, rec->type,
                          (strcmp(rec->scope, "global") == 0 ? "global" : currentScope));
                t->type = rec->type;
            }
            break;
        }
        default:
            break;
        }
        break;

    default:
        break;
    }
}

/* Verifica tipos na árvore */
static void checkNode(TreeNode *t) {
    switch (t->nodekind) {
    case ExpK:
        switch (t->kind.exp) {
        case OpK:
            if (t->child[0] != NULL && t->child[1] != NULL) {
                if (t->child[0]->type != Void && t->child[1]->type != Void) {
                    if ((t->child[0]->type != Integer && t->child[0]->type != Boolean) ||
                        (t->child[1]->type != Integer && t->child[1]->type != Boolean)) {
                        fprintf(listing,
                                "\nERRO SEMANTICO: operacao com tipo invalido - LINHA: %d\n",
                                t->lineno);
                        Error = TRUE;
                    }
                }
            }

            if ((t->attr.op == EQ) || (t->attr.op == NE) ||
                (t->attr.op == LT) || (t->attr.op == LE) ||
                (t->attr.op == GT) || (t->attr.op == GE))
                t->type = Boolean;
            else
                t->type = Integer;
            break;

        case ConstK:
            t->type = Integer;
            break;

        case IdK:
        case ArrIdK:
            if (t->type == Void) t->type = Integer;
            break;

        default:
            break;
        }
        break;

    case StmtK:
        switch (t->kind.stmt) {

        case AssignK: {
            /* redundância de segurança: repete as regras aqui também */
            SymbolRec *rec = findSymbolRecVisible(t->attr.name, currentScope);
            if (rec != NULL) {
                if (rec->isFunction) {
                    fprintf(listing,
                            "\nERRO SEMANTICO: atribuicao a funcao '%s' - LINHA: %d\n",
                            t->attr.name, t->lineno);
                    Error = TRUE;
                } else if (rec->type == IntegerArray) {
                    fprintf(listing,
                            "\nERRO SEMANTICO: atribuicao de indice ao array '%s' (use '%s[i] = ...') - LINHA: %d\n",
                            t->attr.name, t->attr.name, t->lineno);
                    Error = TRUE;
                }
            }

            if (t->child[1] != NULL && t->child[1]->type != Void) {
                if (t->child[1]->type != Integer && t->child[1]->type != Boolean) {
                    fprintf(listing,
                            "\nERRO SEMANTICO: atribuicao com tipo invalido - LINHA: %d\n",
                            t->lineno);
                    Error = TRUE;
                }
            }
            break;
        }

        case IfK:
            if (t->child[0] != NULL && t->child[0]->type != Void) {
                if (t->child[0]->type != Boolean && t->child[0]->type != Integer) {
                    fprintf(listing,
                            "\nERRO SEMANTICO: condicao do if deve ser booleana - LINHA: %d\n",
                            t->lineno);
                    Error = TRUE;
                }
            }
            break;

        case WhileK:
            if (t->child[0] != NULL && t->child[0]->type != Void) {
                if (t->child[0]->type != Boolean && t->child[0]->type != Integer) {
                    fprintf(listing,
                            "\nERRO SEMANTICO: condicao do while deve ser booleana - LINHA: %d\n",
                            t->lineno);
                    Error = TRUE;
                }
            }
            break;

        case CallK: {
            SymbolRec *f = findSymbolRecInScope(t->attr.name, "global");
            if (f == NULL || !f->isFunction) {
                fprintf(listing,
                        "\nERRO SEMANTICO: chamada para funcao nao declarada '%s' - LINHA: %d\n",
                        t->attr.name, t->lineno);
                Error = TRUE;
                t->type = Integer;
                break;
            }

            int got = countArgs(t->child[0]);
            int expected = f->paramCount;

            if (got != expected) {
                fprintf(listing,
                        "\nERRO SEMANTICO: chamada da funcao '%s' com %d argumento(s), esperado %d - LINHA: %d\n",
                        t->attr.name, got, expected, t->lineno);
                Error = TRUE;
            }

            t->type = f->type;
            break;
        }

        default:
            break;
        }
        break;

    default:
        break;
    }
}

void buildSymtab(TreeNode *syntaxTree) {
    symbolList = NULL;
    hasMain = 0;
    location = 0;
    currentScope = "global";

    insertBuiltins();
    traverse(syntaxTree, insertNode, afterNode);

    currentScope = "global";

    if (!hasMain) {
        fprintf(listing,
                "\nERRO SEMANTICO: programa deve conter uma funcao 'void main(void)'\n");
        Error = TRUE;
    }
}

void typeCheck(TreeNode *syntaxTree) {
    currentScope = "global";
    traverse(syntaxTree, enterScopeForTypeCheck, checkNode);
    currentScope = "global";
}