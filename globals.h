/*
 * Definições globais do compilador C-
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0

#define MAXRESERVED 6
#define MAXCHILDREN 3

extern int lineno;

typedef enum {
    // Palavras reservadas
    IF, ELSE, INT, RETURN, VOID, WHILE,
    // Tokens especiais
    PLUS, MINUS, TIMES, OVER, LT, LE, GT, GE, EQ, NE,
    ASSIGN, SEMI, COMMA, LPAREN, RPAREN, LBRACKET, RBRACKET,
    LBRACE, RBRACE,
    // Outros
    ID, NUM, ENDFILE, ERROR
} TokenType;

extern FILE *source;
extern FILE *listing;
extern int lineno;

typedef enum {StmtK, ExpK} NodeKind;

typedef enum {
    IfK, WhileK, AssignK, ReturnK, CompoundK, FunDeclK, VarDeclK, ParamK, CallK
} StmtKind;

typedef enum {OpK, ConstK, IdK, ArrIdK} ExpKind;

typedef enum {Void, Integer, IntegerArray, Boolean} ExpType;

typedef struct treeNode {
    struct treeNode *child[MAXCHILDREN];
    struct treeNode *sibling;
    int lineno;
    NodeKind nodekind;
    union {
        StmtKind stmt;
        ExpKind exp;
    } kind;
    union {
        TokenType op;
        int val;
        char *name;
    } attr;
    ExpType type;
    int arraySize;
} TreeNode;

extern int EchoSource;
extern int TraceScan;
extern int TraceParse;
extern int TraceAnalyze;
extern int TraceCode;
extern int Error;

#endif