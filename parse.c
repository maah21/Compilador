/* Parser (Analisador Sintático) para C-*/

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TokenType token;
static int errorCount = 0;
static const int MAX_ERRORS = 1;

static TreeNode *declaration_list(void);
static TreeNode *declaration(void);
static TreeNode *var_declaration(void);
static TreeNode *fun_declaration(void);
static TreeNode *params(void);
static TreeNode *param_list(void);
static TreeNode *param(void);
static TreeNode *compound_stmt(void);
static TreeNode *local_declarations(void);
static TreeNode *statement_list(void);
static TreeNode *statement(void);
static TreeNode *expression_stmt(void);
static TreeNode *conditional_expression(void);
static TreeNode *selection_stmt(void);
static TreeNode *iteration_stmt(void);
static TreeNode *return_stmt(void);
static TreeNode *expression(void);
static TreeNode *simple_expression(TreeNode *);
static TreeNode *additive_expression(TreeNode *);
static TreeNode *term(TreeNode *);
static TreeNode *factor(void);
static TreeNode *call(char *);
static TreeNode *args(void);
static TreeNode *arg_list(void);

/* helper: parse de expressão quando statement começou com ID e já consumimos o ID */
static TreeNode *expression_from_consumed_id(char *identifier);

static void abortCompilation(void) {
    fprintf(listing, "\n========================================\n");
    fprintf(listing, "COMPILACAO ABORTADA: Erros detectados na analise\n");
    fprintf(listing, "========================================\n");
    exit(1);
}

/* ---------------------- Padronização de mensagens ---------------------- */

static const char *expectedTokenStr(TokenType t) {
    switch (t) {
        case SEMI:     return "';'";
        case LPAREN:   return "'('";
        case RPAREN:   return "')'";
        case LBRACKET: return "'['";
        case RBRACKET: return "']'";
        case LBRACE:   return "'{'";
        case RBRACE:   return "'}'";
        case ASSIGN:   return "'='";
        case COMMA:    return "','";
        case IF:       return "'if'";
        case ELSE:     return "'else'";
        case WHILE:    return "'while'";
        case RETURN:   return "'return'";
        case INT:      return "'int'";
        case VOID:     return "'void'";
        case ID:       return "identificador";
        case NUM:      return "numero";
        case ENDFILE:  return "fim de arquivo";
        default:       return "token";
    }
}

static void foundTokenStr(char *out, size_t outSz) {
    switch (token) {
        case ID:
        case NUM:
        case ERROR:
            snprintf(out, outSz, "'%s'", stringToken);
            break;
        case ENDFILE:
            snprintf(out, outSz, "fim de arquivo");
            break;
        default:
            snprintf(out, outSz, "'%s'", stringToken);
            break;
    }
}

static void syntaxUnexpectedExpectedStr(const char *expectedStr) {
    if (errorCount >= MAX_ERRORS) return;

    char foundStr[128];
    foundTokenStr(foundStr, sizeof(foundStr));

    fprintf(listing,
            "\nERRO SINTATICO: token inesperado %s, esperado '%s' - LINHA: %d\n",
            foundStr, expectedStr, lineno);

    Error = TRUE;
    errorCount++;

    if (errorCount >= MAX_ERRORS) abortCompilation();
}

static void syntaxUnexpectedToken(TokenType expectedTok) {
    syntaxUnexpectedExpectedStr(expectedTokenStr(expectedTok));
}

static void match(TokenType expected) {
    if (token == expected) {
        token = getToken();
    } else {
        syntaxUnexpectedToken(expected);
    }
}

/* ---------------------- Parser ---------------------- */

TreeNode *declaration_list(void) {
    TreeNode *t = declaration();
    TreeNode *p = t;

    while (token != ENDFILE && errorCount < MAX_ERRORS) {
        TreeNode *q = declaration();
        if (q != NULL) {
            if (t == NULL) t = p = q;
            else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

TreeNode *declaration(void) {
    TreeNode *t = NULL;
    ExpType type;
    char *identifier;

    if (token == INT) {
        type = Integer;
        match(INT);
    } else if (token == VOID) {
        type = Void;
        match(VOID);
    } else {
        syntaxUnexpectedExpectedStr("tipo ('int' ou 'void')");
        return NULL;
    }

    if (token == ID) {
        identifier = copyString(stringToken);
        match(ID);
    } else {
        syntaxUnexpectedToken(ID);
        return NULL;
    }

    if (token == LPAREN) {
        t = fun_declaration();
        if (t != NULL) {
            t->type = type;
            t->attr.name = identifier;
        }
    } else {
        t = var_declaration();
        if (t != NULL) {
            /* var_declaration() já define IntegerArray quando vê [NUM] */
            if (t->type != IntegerArray) {
                t->type = type;
            } else {
                /* array em C- deve ser int; se vier void, pode acusar (opcional) */
                if (type == Void) {
                    /* syntaxUnexpectedExpectedStr("tipo 'int' para array"); */
                }
            }
            t->attr.name = identifier;
        }
    }

    return t;
}

TreeNode *var_declaration(void) {
    TreeNode *t = newStmtNode(VarDeclK);

    if (token == LBRACKET) {
        match(LBRACKET);

        if (token == NUM) {
            t->arraySize = atoi(stringToken);
            t->type = IntegerArray;
            match(NUM);
        } else {
            syntaxUnexpectedExpectedStr("tamanho do array (numero)");
        }

        match(RBRACKET);
    }

    match(SEMI);
    return t;
}

TreeNode *fun_declaration(void) {
    TreeNode *t = newStmtNode(FunDeclK);
    match(LPAREN);
    t->child[0] = params();
    match(RPAREN);
    t->child[1] = compound_stmt();
    return t;
}

TreeNode *params(void) {
    TreeNode *t = NULL;
    if (token == VOID) {
        match(VOID);
    } else {
        t = param_list();
    }
    return t;
}

TreeNode *param_list(void) {
    TreeNode *t = param();
    TreeNode *p = t;

    while (token == COMMA) {
        match(COMMA);
        TreeNode *q = param();
        if (q != NULL) {
            if (t == NULL) t = p = q;
            else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

TreeNode *param(void) {
    TreeNode *t = newStmtNode(ParamK);

    if (token == INT) t->type = Integer;
    else if (token == VOID) t->type = Void;
    else {
        syntaxUnexpectedExpectedStr("tipo ('int' ou 'void')");
        return t;
    }
    match(token);

    if (token == ID) {
        t->attr.name = copyString(stringToken);
        match(ID);
    } else {
        syntaxUnexpectedToken(ID);
        return t;
    }

    if (token == LBRACKET) {
        match(LBRACKET);
        match(RBRACKET);
        t->type = IntegerArray;
    }

    return t;
}

TreeNode *compound_stmt(void) {
    TreeNode *t = newStmtNode(CompoundK);
    match(LBRACE);
    t->child[0] = local_declarations();
    t->child[1] = statement_list();
    match(RBRACE);
    return t;
}

TreeNode *local_declarations(void) {
    TreeNode *t = NULL;
    TreeNode *p = NULL;

    while ((token == INT || token == VOID) && errorCount < MAX_ERRORS) {
        TreeNode *q = declaration();
        if (q != NULL) {
            if (t == NULL) t = p = q;
            else {
                p->sibling = q;
                p = q;
            }
        }
    }

    return t;
}

TreeNode *statement_list(void) {
    TreeNode *t = statement();
    TreeNode *p = t;

    while (token != RBRACE && token != ENDFILE && errorCount < MAX_ERRORS) {
        TreeNode *q = statement();
        if (q != NULL) {
            if (t == NULL) t = p = q;
            else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

TreeNode *statement(void) {
    TreeNode *t = NULL;

    switch (token) {
        case IF:     t = selection_stmt(); break;
        case WHILE:  t = iteration_stmt(); break;
        case RETURN: t = return_stmt(); break;
        case LBRACE: t = compound_stmt(); break;

        case ID:
        case LPAREN:
        case NUM:
        case SEMI:
            t = expression_stmt();
            break;

        default:
            syntaxUnexpectedExpectedStr("inicio de comando (if/while/return/bloco/expressao)");
            break;
    }
    return t;
}

TreeNode *expression_stmt(void) {
    TreeNode *t = NULL;

    if (token == SEMI) {
        match(SEMI);
        return NULL;
    }

    if (token == ID) {
        char *identifier = copyString(stringToken);
        match(ID);

        /* Se veio ';' direto: exemplo "add;" => erro */
        if (token == SEMI) {
            syntaxUnexpectedExpectedStr("'(', '[', '='");
            return NULL;
        }

        t = expression_from_consumed_id(identifier);
        match(SEMI);
        return t;
    }

    t = expression();
    match(SEMI);
    return t;
}

TreeNode *conditional_expression(void) {
    TreeNode *t = NULL;

    if (token == ID) {
        char *identifier = copyString(stringToken);
        match(ID);

        if (token == ASSIGN) {
            syntaxUnexpectedExpectedStr("'==','<=','>=', '>' ou '<'");
            return NULL;
        }

        if (token == LBRACKET) {
            TreeNode *arr = newExpNode(ArrIdK);
            arr->attr.name = identifier;
            match(LBRACKET);
            arr->child[0] = expression();
            match(RBRACKET);

            if (token == ASSIGN) {
                syntaxUnexpectedExpectedStr("'==','<=','>=', '>' ou '<'");
                return NULL;
            }

            t = simple_expression(arr);
        }
        else if (token == LPAREN) {
            t = call(identifier);
            t = simple_expression(t);
        }
        else {
            TreeNode *id = newExpNode(IdK);
            id->attr.name = identifier;
            t = simple_expression(id);
        }
    } else {
        t = simple_expression(NULL);
    }

    return t;
}

TreeNode *selection_stmt(void) {
    TreeNode *t = newStmtNode(IfK);
    match(IF);
    match(LPAREN);
    t->child[0] = conditional_expression();
    match(RPAREN);
    t->child[1] = statement();
    if (token == ELSE) {
        match(ELSE);
        t->child[2] = statement();
    }
    return t;
}

TreeNode *iteration_stmt(void) {
    TreeNode *t = newStmtNode(WhileK);
    match(WHILE);
    match(LPAREN);
    t->child[0] = conditional_expression();
    match(RPAREN);
    t->child[1] = statement();
    return t;
}

TreeNode *return_stmt(void) {
    TreeNode *t = newStmtNode(ReturnK);
    match(RETURN);
    if (token != SEMI) {
        t->child[0] = expression();
    }
    match(SEMI);
    return t;
}

TreeNode *expression(void) {
    TreeNode *t = NULL;

    if (token == ID) {
        char *identifier = copyString(stringToken);

        /* input/output exigem '(' depois */
        if (strcmp(stringToken, "input") == 0 || strcmp(stringToken, "output") == 0) {
            match(ID);
            if (token != LPAREN) {
                syntaxUnexpectedToken(LPAREN);
                return NULL;
            }
            t = call(identifier);
            t = simple_expression(t);
            return t;
        }

        match(ID);

        if (token == LBRACKET) {
            TreeNode *arr = newExpNode(ArrIdK);
            arr->attr.name = identifier;
            match(LBRACKET);
            arr->child[0] = expression();
            match(RBRACKET);

            if (token == ASSIGN) {
                t = newStmtNode(AssignK);
                t->attr.name = identifier;
                t->child[0] = arr;
                match(ASSIGN);
                t->child[1] = expression();
            } else {
                t = simple_expression(arr);
            }
        } else if (token == ASSIGN) {
            t = newStmtNode(AssignK);
            t->attr.name = identifier;
            match(ASSIGN);
            t->child[1] = expression();
        } else if (token == LPAREN) {
            t = call(identifier);
            t = simple_expression(t);
        } else {
            TreeNode *id = newExpNode(IdK);
            id->attr.name = identifier;
            t = simple_expression(id);
        }
    } else {
        t = simple_expression(NULL);
    }

    return t;
}

/* helper: mesma lógica do começo de expression(), mas com ID já consumido */
static TreeNode *expression_from_consumed_id(char *identifier) {

    if (strcmp(identifier, "input") == 0 || strcmp(identifier, "output") == 0) {
        if (token != LPAREN) {
            syntaxUnexpectedToken(LPAREN);
            return NULL;
        }
        TreeNode *t = call(identifier);
        return simple_expression(t);
    }

    if (token == LBRACKET) {
        TreeNode *arr = newExpNode(ArrIdK);
        arr->attr.name = identifier;
        match(LBRACKET);
        arr->child[0] = expression();
        match(RBRACKET);

        if (token == ASSIGN) {
            TreeNode *t = newStmtNode(AssignK);
            t->attr.name = identifier;
            t->child[0] = arr;
            match(ASSIGN);
            t->child[1] = expression();
            return t;
        }

        return simple_expression(arr);
    }

    if (token == ASSIGN) {
        TreeNode *t = newStmtNode(AssignK);
        t->attr.name = identifier;
        match(ASSIGN);
        t->child[1] = expression();
        return t;
    }

    if (token == LPAREN) {
        TreeNode *t = call(identifier);
        return simple_expression(t);
    }

    TreeNode *id = newExpNode(IdK);
    id->attr.name = identifier;
    return simple_expression(id);
}

TreeNode *simple_expression(TreeNode *k) {
    TreeNode *t = additive_expression(k);

    if (token == LE || token == LT || token == GT ||
        token == GE || token == EQ || token == NE) {
        TreeNode *p = newExpNode(OpK);
        p->attr.op = token;
        p->child[0] = t;
        t = p;
        match(token);
        t->child[1] = additive_expression(NULL);
    }

    return t;
}

TreeNode *additive_expression(TreeNode *k) {
    TreeNode *t = term(k);

    while (token == PLUS || token == MINUS) {
        TreeNode *p = newExpNode(OpK);
        p->attr.op = token;
        p->child[0] = t;
        t = p;
        match(token);
        p->child[1] = term(NULL);
    }

    return t;
}

TreeNode *term(TreeNode *k) {
    TreeNode *t = (k != NULL) ? k : factor();

    while (token == TIMES || token == OVER) {
        TreeNode *p = newExpNode(OpK);
        p->attr.op = token;
        p->child[0] = t;
        t = p;
        match(token);
        p->child[1] = factor();
    }

    return t;
}

TreeNode *factor(void) {
    TreeNode *t = NULL;

    switch (token) {
        case NUM:
            t = newExpNode(ConstK);
            t->attr.val = atoi(stringToken);
            match(NUM);
            break;

        case ID: {
            char *identifier = copyString(stringToken);

            if (strcmp(stringToken, "input") == 0 || strcmp(stringToken, "output") == 0) {
                match(ID);
                if (token != LPAREN) {
                    syntaxUnexpectedToken(LPAREN);
                    return NULL;
                }
                t = call(identifier);
                return t;
            }

            match(ID);

            if (token == LPAREN) {
                t = call(identifier);
            } else if (token == LBRACKET) {
                t = newExpNode(ArrIdK);
                t->attr.name = identifier;
                match(LBRACKET);
                t->child[0] = expression();
                match(RBRACKET);
            } else {
                t = newExpNode(IdK);
                t->attr.name = identifier;
            }
            break;
        }

        case LPAREN:
            match(LPAREN);
            t = expression();
            match(RPAREN);
            break;

        default:
            syntaxUnexpectedExpectedStr("fator (NUM, ID ou '(')");
            return NULL;
    }

    return t;
}

TreeNode *call(char *identifier) {
    TreeNode *t = newStmtNode(CallK);
    t->attr.name = identifier;

    match(LPAREN);
    t->child[0] = args();
    match(RPAREN);

    return t;
}

TreeNode *args(void) {
    if (token != RPAREN) {
        return arg_list();
    }
    return NULL;
}

TreeNode *arg_list(void) {
    TreeNode *t = expression();
    TreeNode *p = t;

    while (token == COMMA) {
        match(COMMA);
        TreeNode *q = expression();
        if (q != NULL) {
            if (t == NULL) t = p = q;
            else {
                p->sibling = q;
                p = q;
            }
        }
    }

    return t;
}

TreeNode *parse(void) {
    TreeNode *t;
    errorCount = 0;

    token = getToken();
    t = declaration_list();

    if (token != ENDFILE && errorCount < MAX_ERRORS) {
        syntaxUnexpectedExpectedStr("fim de arquivo");
    }

    return t;
}