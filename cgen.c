/*Gerador de Código Intermediário (AST Linearizada)*/

#include "globals.h"
#include "util.h"
#include "cgen.h"

static int tempCounter = 0;
static int labelCounter = 0;

/*Gera novo temporário*/
static char *newTemp(void) {
    static char temp[20];
    sprintf(temp, "t%d", tempCounter++);
    return copyString(temp);
}

/*Gera novo label*/
static char *newLabel(void) {
    static char label[20];
    sprintf(label, "L%d", labelCounter++);
    return copyString(label);
}

/*Emite label*/
static void emitLabel(char *label) {
    fprintf(listing, "%s:\n", label);
}

/*Emite goto*/
static void emitGoto(char *label) {
    fprintf(listing, "goto %s\n", label);
}

/*Emite salto condicional*/
static void emitIfFalse(char *cond, char *label) {
    fprintf(listing, "if_false %s goto %s\n", cond, label);
}

/* Geração de código para expressões - retorna temporário com resultado */
static char *cGen(TreeNode *tree) {
    if (tree == NULL) return NULL;
    
    char *t1, *t2, *t3;
    char numStr[20];
    
    switch (tree->nodekind) {
    case ExpK:
        switch (tree->kind.exp) {
        case ConstK:
            sprintf(numStr, "%d", tree->attr.val);
            return copyString(numStr);
            
        case IdK:
            return tree->attr.name;
            
        case ArrIdK:
            t1 = cGen(tree->child[0]); /* índice */
            t2 = newTemp();
            fprintf(listing, "%s = %s[%s]\n", t2, tree->attr.name, t1);
            return t2;
            
        case OpK:
            t1 = cGen(tree->child[0]);
            t2 = cGen(tree->child[1]);
            t3 = newTemp();
            
            char *opStr;
            switch (tree->attr.op) {
            case PLUS: opStr = "+"; break;
            case MINUS: opStr = "-"; break;
            case TIMES: opStr = "*"; break;
            case OVER: opStr = "/"; break;
            case LT: opStr = "<"; break;
            case LE: opStr = "<="; break;
            case GT: opStr = ">"; break;
            case GE: opStr = ">="; break;
            case EQ: opStr = "=="; break;
            case NE: opStr = "!="; break;
            default: opStr = "?"; break;
            }
            
            fprintf(listing, "%s = %s %s %s\n", t3, t1, opStr, t2);
            return t3;
            
        default:
            return NULL;
        }
        break;
        
    case StmtK:
        switch (tree->kind.stmt) {
        case AssignK:
            if (tree->child[0] != NULL && tree->child[0]->kind.exp == ArrIdK) {
                /* Atribuição a array: arr[i] = expr */
                t1 = cGen(tree->child[0]->child[0]); /* índice */
                t2 = cGen(tree->child[1]); /* valor */
                fprintf(listing, "%s[%s] = %s\n", tree->attr.name, t1, t2);
            } else {
                /* Atribuição simples: var = expr */
                t1 = cGen(tree->child[1]);
                fprintf(listing, "%s = %s\n", tree->attr.name, t1);
            }
            break;
            
        case IfK:
            {
                char *labelElse = newLabel();
                char *labelEnd = newLabel();
                
                t1 = cGen(tree->child[0]); /* condição */
                emitIfFalse(t1, labelElse);
                
                /* Bloco then */
                if (tree->child[1] != NULL)
                    cGen(tree->child[1]);
                
                if (tree->child[2] != NULL) {
                    emitGoto(labelEnd);
                    emitLabel(labelElse);
                    /* Bloco else */
                    cGen(tree->child[2]);
                    emitLabel(labelEnd);
                } else {
                    emitLabel(labelElse);
                }
            }
            break;
            
        case WhileK:
            {
                char *labelStart = newLabel();
                char *labelEnd = newLabel();
                
                emitLabel(labelStart);
                t1 = cGen(tree->child[0]); /*condição */
                emitIfFalse(t1, labelEnd);
                
                /* Corpo do loop*/
                if (tree->child[1] != NULL)
                    cGen(tree->child[1]);
                
                emitGoto(labelStart);
                emitLabel(labelEnd);
            }
            break;
            
        case ReturnK:
            if (tree->child[0] != NULL) {
                t1 = cGen(tree->child[0]);
                fprintf(listing, "return %s\n", t1);
            } else {
                fprintf(listing, "return\n");
            }
            break;
            
        case CallK:
            {
                /*Processa argumentos*/
                TreeNode *arg = tree->child[0];
                while (arg != NULL) {
                    t1 = cGen(arg);
                    fprintf(listing, "param %s\n", t1);
                    arg = arg->sibling;
                }
                
                t2 = newTemp();
                fprintf(listing, "%s = call %s\n", t2, tree->attr.name);
                return t2;
            }
            
        case FunDeclK:
            {
                fprintf(listing, "\nfunc %s:\n", tree->attr.name);
                
                /*Parâmetros*/
                TreeNode *param = tree->child[0];
                while (param != NULL) {
                    fprintf(listing, "param %s\n", param->attr.name);
                    param = param->sibling;
                }
                
                /*Corpo da função */
                if (tree->child[1] != NULL)
                    cGen(tree->child[1]);
                
                fprintf(listing, "endfunc %s\n", tree->attr.name);
            }
            break;
            
        case VarDeclK:
            if (tree->type == IntegerArray) {
                fprintf(listing, "var %s[%d]\n", tree->attr.name, tree->arraySize);
            } else {
                fprintf(listing, "var %s\n", tree->attr.name);
            }
            break;
            
        case CompoundK:
            if (tree->child[0] != NULL) /*declarações locais*/
                cGen(tree->child[0]);
            if (tree->child[1] != NULL) /*lista de statements*/
                cGen(tree->child[1]);
            break;
            
        default:
            break;
        }
        break;
        
    default:
        break;
    }
    
    /*Processa irmãos*/
    if (tree->sibling != NULL) {
        cGen(tree->sibling);
    }
    
    return NULL;
}

/*Gera código para árvore completa */
void codeGen(TreeNode *syntaxTree) {
    fprintf(listing, "\n>>> Codigo Intermediario (AST Linearizada) <<<\n\n");
    tempCounter = 0;
    labelCounter = 0;
    
    /* cGen já processa irmãos internamente, então só chamamos uma vez */
    cGen(syntaxTree);
    
    fprintf(listing, "\n>>> Fim do Codigo Intermediario <<<\n");
}