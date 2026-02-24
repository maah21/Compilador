/*Funções utilitárias para o compilador C- (util.c)*/

#include "globals.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* IMPORTANTE: A variável global é stringToken (definida em scan.c) */
extern char stringToken[];

void printToken(TokenType token, const char *tokenStr) {
    switch (token) {
    case IF:
    case ELSE:
    case INT:
    case RETURN:
    case VOID:
    case WHILE:
        fprintf(listing, "palavra-chave: %s\n", tokenStr);
        break;
    case ASSIGN: fprintf(listing, "=\n"); break;
    case EQ: fprintf(listing, "==\n"); break;
    case NE: fprintf(listing, "!=\n"); break;
    case LT: fprintf(listing, "<\n"); break;
    case LE: fprintf(listing, "<=\n"); break;
    case GT: fprintf(listing, ">\n"); break;
    case GE: fprintf(listing, ">=\n"); break;
    case PLUS: fprintf(listing, "+\n"); break;
    case MINUS: fprintf(listing, "-\n"); break;
    case TIMES: fprintf(listing, "*\n"); break;
    case OVER: fprintf(listing, "/\n"); break;
    case LPAREN: fprintf(listing, "(\n"); break;
    case RPAREN: fprintf(listing, ")\n"); break;
    case LBRACKET: fprintf(listing, "[\n"); break;
    case RBRACKET: fprintf(listing, "]\n"); break;
    case LBRACE: fprintf(listing, "{\n"); break;
    case RBRACE: fprintf(listing, "}\n"); break;
    case SEMI: fprintf(listing, ";\n"); break;
    case COMMA: fprintf(listing, ",\n"); break;
    case ENDFILE: fprintf(listing, "EOF\n"); break;
    case NUM:
        fprintf(listing, "NUM: %s\n", tokenStr);
        break;
    case ID:
        fprintf(listing, "ID: %s\n", tokenStr);
        break;
    case ERROR:
        fprintf(listing, "ERRO: %s\n", tokenStr);
        break;
    default:
        fprintf(listing, "Token desconhecido: %d\n", token);
    }
}

TreeNode *newStmtNode(StmtKind kind) {
    TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
    int i;
    if (t == NULL)
        fprintf(listing, "Erro: sem memoria\n");
    else {
        for (i = 0; i < MAXCHILDREN; i++) t->child[i] = NULL;
        t->sibling = NULL;
        t->nodekind = StmtK;
        t->kind.stmt = kind;
        t->lineno = lineno;
        t->type = Void;
        t->arraySize = 0;
        t->attr.name = NULL;
    }
    return t;
}

TreeNode *newExpNode(ExpKind kind) {
    TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
    int i;
    if (t == NULL)
        fprintf(listing, "Erro: sem memoria\n");
    else {
        for (i = 0; i < MAXCHILDREN; i++) t->child[i] = NULL;
        t->sibling = NULL;
        t->nodekind = ExpK;
        t->kind.exp = kind;
        t->lineno = lineno;
        t->type = Void;
        t->arraySize = 0;
        t->attr.name = NULL;
    }
    return t;
}

char *copyString(char *s) {
    int n;
    char *t;
    if (s == NULL) return NULL;
    n = (int)strlen(s) + 1;
    t = (char*)malloc(n);
    if (t == NULL)
        fprintf(listing, "Erro: sem memoria\n");
    else
        strcpy(t, s);
    return t;
}


static int nodeCounter = 0;
static FILE *dotFile = NULL;

static const char* getNodeColor(TreeNode *tree) {
    if (tree->nodekind == StmtK) {
        switch (tree->kind.stmt) {
        case FunDeclK: return "lightgreen";
        case IfK:
        case WhileK: return "lightyellow";
        case AssignK: return "lightcoral";
        case ReturnK: return "pink";
        case VarDeclK:
        case ParamK: return "lightgray";
        case CallK: return "plum";
        case CompoundK: return "wheat";
        default: return "lightblue";
        }
    } else if (tree->nodekind == ExpK) {
        if (tree->kind.exp == OpK) return "orange";
        if (tree->kind.exp == ConstK) return "white";
        return "lightcyan";
    }
    return "lightblue";
}

static const char* getNodeShape(TreeNode *tree) {
    if (tree->nodekind == StmtK) {
        switch (tree->kind.stmt) {
        case FunDeclK:
        case VarDeclK:
        case ParamK: return "box";
        case IfK:
        case WhileK: return "diamond";
        default: return "ellipse";
        }
    } else if (tree->nodekind == ExpK) {
        if (tree->kind.exp == OpK) return "circle";
        if (tree->kind.exp == ConstK) return "box";
        return "ellipse";
    }
    return "ellipse";
}

static void escapeLabel(char *dest, const char *src, int maxLen) {
    int i = 0, j = 0;
    while (src[i] != '\0' && j < maxLen - 1) {
        if (src[i] == '"') {
            dest[j++] = '\\';
            dest[j++] = '"';
        } else if (src[i] == '\\') {
            dest[j++] = '\\';
            dest[j++] = '\\';
        } else if (src[i] == '\n') {
            dest[j++] = '\\';
            dest[j++] = 'n';
        } else {
            dest[j++] = src[i];
        }
        i++;
    }
    dest[j] = '\0';
}

static void getNodeLabel(TreeNode *tree, char *label, int maxLen) {
    char temp[100];

    if (tree == NULL) {
        snprintf(label, maxLen, "NULL");
        return;
    }

    if (tree->nodekind == StmtK) {
        switch (tree->kind.stmt) {
        case IfK:
            snprintf(label, maxLen, "if");
            break;
        case WhileK:
            snprintf(label, maxLen, "while");
            break;
        case AssignK:
            snprintf(label, maxLen, "=");
            break;
        case ReturnK:
            snprintf(label, maxLen, "return");
            break;
        case FunDeclK:
            if (tree->attr.name != NULL) {
                escapeLabel(temp, tree->attr.name, (int)sizeof(temp));
                snprintf(label, maxLen, "%s : %s", temp,
                         tree->type == Integer ? "int" : "void");
            } else {
                snprintf(label, maxLen, "func : %s",
                         tree->type == Integer ? "int" : "void");
            }
            break;
        case VarDeclK:
            if (tree->attr.name != NULL) {
                escapeLabel(temp, tree->attr.name, (int)sizeof(temp));
                if (tree->type == IntegerArray)
                    snprintf(label, maxLen, "int %s[%d]", temp, tree->arraySize);
                else
                    snprintf(label, maxLen, "int %s", temp);
            } else {
                snprintf(label, maxLen, "int var");
            }
            break;
        case ParamK:
            if (tree->attr.name != NULL) {
                escapeLabel(temp, tree->attr.name, (int)sizeof(temp));
                if (tree->type == IntegerArray)
                    snprintf(label, maxLen, "%s[]", temp);
                else
                    snprintf(label, maxLen, "%s", temp);
            } else {
                snprintf(label, maxLen, "param");
            }
            break;
        case CallK:
            if (tree->attr.name != NULL) {
                escapeLabel(temp, tree->attr.name, (int)sizeof(temp));
                snprintf(label, maxLen, "%s", temp);
            } else {
                snprintf(label, maxLen, "call");
            }
            break;
        case CompoundK:
            snprintf(label, maxLen, "compound");
            break;
        default:
            snprintf(label, maxLen, "?stmt?");
            break;
        }
    } else if (tree->nodekind == ExpK) {
        switch (tree->kind.exp) {
        case OpK:
            switch (tree->attr.op) {
            case PLUS:  snprintf(label, maxLen, "+");  break;
            case MINUS: snprintf(label, maxLen, "-");  break;
            case TIMES: snprintf(label, maxLen, "*");  break;
            case OVER:  snprintf(label, maxLen, "/");  break;
            case LT:    snprintf(label, maxLen, "<");  break;
            case LE:    snprintf(label, maxLen, "<="); break;
            case GT:    snprintf(label, maxLen, ">");  break;
            case GE:    snprintf(label, maxLen, ">="); break;
            case EQ:    snprintf(label, maxLen, "=="); break;
            case NE:    snprintf(label, maxLen, "!="); break;
            default:    snprintf(label, maxLen, "?op?"); break;
            }
            break;
        case ConstK:
            snprintf(label, maxLen, "%d", tree->attr.val);
            break;
        case IdK:
            if (tree->attr.name != NULL) {
                escapeLabel(temp, tree->attr.name, (int)sizeof(temp));
                snprintf(label, maxLen, "%s", temp);
            } else {
                snprintf(label, maxLen, "id");
            }
            break;
        case ArrIdK:
            if (tree->attr.name != NULL) {
                escapeLabel(temp, tree->attr.name, (int)sizeof(temp));
                snprintf(label, maxLen, "%s[]", temp);
            } else {
                snprintf(label, maxLen, "arr");
            }
            break;
        default:
            snprintf(label, maxLen, "?exp?");
            break;
        }
    } else {
        snprintf(label, maxLen, "?node?");
    }
}

/* Rótulos das arestas para facilitar visualizar where is what */
static const char* childEdgeLabel(TreeNode *parent, int idx) {
    if (!parent) return "";

    if (parent->nodekind == StmtK) {
        switch (parent->kind.stmt) {
        case IfK:
            if (idx == 0) return "cond";
            if (idx == 1) return "then";
            if (idx == 2) return "else";
            break;
        case WhileK:
            if (idx == 0) return "cond";
            if (idx == 1) return "body";
            break;
        case AssignK:
            if (idx == 0) return "lhs";
            if (idx == 1) return "rhs";
            break;
        case ReturnK:
            if (idx == 0) return "expr";
            break;
        case CompoundK:
            if (idx == 0) return "decls";
            if (idx == 1) return "stmts";
            break;
        case CallK:
            if (idx == 0) return "args";
            break;
        default:
            break;
        }
    } else if (parent->nodekind == ExpK) {
        if (parent->kind.exp == OpK) {
            if (idx == 0) return "left";
            if (idx == 1) return "right";
        } else if (parent->kind.exp == ArrIdK) {
            if (idx == 0) return "index";
        }
    }

    return "";
}

static int printDotNode(TreeNode *tree) {
    if (tree == NULL) return -1;

    int myId = nodeCounter++;
    char label[100];

    getNodeLabel(tree, label, (int)sizeof(label));

    fprintf(dotFile,
        "  node%d [label=\"%s\", shape=%s, style=filled, fillcolor=%s];\n",
        myId, label, getNodeShape(tree), getNodeColor(tree));

    for (int i = 0; i < MAXCHILDREN; i++) {
        if (tree->child[i] != NULL) {
            int childId = printDotNode(tree->child[i]);
            const char *elab = childEdgeLabel(tree, i);

            if (elab[0] != '\0') {
                fprintf(dotFile, "  node%d -> node%d [label=\"%s\"];\n", myId, childId, elab);
            } else {
                fprintf(dotFile, "  node%d -> node%d;\n", myId, childId);
            }
        }
    }

    /* irmãos como lista encadeada (next) */
    if (tree->sibling != NULL) {
        int sibId = printDotNode(tree->sibling);
        fprintf(dotFile, "  node%d -> node%d [style=dashed, label=\"next\"];\n", myId, sibId);
    }

    return myId;
}

void printTreeDot(TreeNode *tree, const char *dotFilename, const char *pngFilename) {
    FILE *f = NULL;

    fprintf(listing, "\n=== GERACAO DO ARQUIVO .DOT ===\n");
    fprintf(listing, "Criando: %s\n", dotFilename);

    f = fopen(dotFilename, "w");

    if (f == NULL) {
        fprintf(listing, "ERRO: Nao foi possivel criar arquivo %s\n", dotFilename);
        fprintf(listing, "Verifique permissoes no diretorio atual\n\n");
        return;
    }

    fprintf(listing, "Arquivo criado com sucesso!\n");

    dotFile = f;

    fprintf(dotFile, "digraph AST {\n");
    fprintf(dotFile, "  rankdir=TB;\n");
    fprintf(dotFile, "  node [fontname=\"Arial\", fontsize=12];\n");
    fprintf(dotFile, "  edge [color=black, penwidth=1.5];\n\n");

    nodeCounter = 0;
    printDotNode(tree);

    fprintf(dotFile, "}\n");
    fclose(dotFile);

    fprintf(listing, "\n=== ARQUIVO .DOT GERADO ===\n");
    fprintf(listing, "Arquivo DOT: %s\n", dotFilename);

    fprintf(listing, "\nGerando PNG...\n");
    char cmd[512];
    snprintf(cmd, (int)sizeof(cmd), "dot -Tpng \"%s\" -o \"%s\" 2>nul", dotFilename, pngFilename);
    int ret = system(cmd);

    if (ret == 0) {
        fprintf(listing, "Arquivo PNG gerado: %s\n\n", pngFilename);
    } else {
        fprintf(listing, "Aviso: Nao foi possivel gerar PNG automaticamente\n");
        fprintf(listing, "\nPara gerar manualmente, execute:\n");
        fprintf(listing, "========================================\n");
        fprintf(listing, "  dot -Tpng \"%s\" -o \"%s\"\n", dotFilename, pngFilename);
        fprintf(listing, "========================================\n");
        fprintf(listing, "\nSe o comando 'dot' nao for reconhecido:\n");
        fprintf(listing, "  1. Instale Graphviz: https://graphviz.org/download/\n");
        fprintf(listing, "  2. Adicione ao PATH: C:\\Program Files\\Graphviz\\bin\n");
        fprintf(listing, "  3. Reinicie o terminal e execute o comando acima\n\n");
    }
}