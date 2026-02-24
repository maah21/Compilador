/* scan.c - Scanner */

#include "globals.h"
#include "util.h"
#include "scan.h"

#include <ctype.h>
#include <string.h>

typedef enum {
    INICIO, EMCOMENTARIO, EMNUM, EMNUMERRO, EMID, EMATRIBUICAO, EMNE, EMLE, EMGE, CONCLUIDO
} TipoEstado;

char stringToken[MAXTOKENLEN + 1];

#define TAMANHOBUF 256
static char bufferLinha[TAMANHOBUF];
static int posicaoLinha = 0;
static int tamanhoBuffer = 0;
static int flag_EOF = FALSE;

static int linhaInicioComentario = 0;

static struct {
    char *texto;
    TokenType token;
} palavrasReservadas[MAXRESERVED] = {
    {"else", ELSE}, {"if", IF}, {"int", INT},
    {"return", RETURN}, {"void", VOID}, {"while", WHILE}
};

static int obterProximoChar(void) {
    if (!(posicaoLinha < tamanhoBuffer)) {
        lineno++;
        if (fgets(bufferLinha, TAMANHOBUF - 1, source)) {
            if (EchoSource) fprintf(listing, "%4d: %s", lineno, bufferLinha);
            tamanhoBuffer = (int)strlen(bufferLinha);
            posicaoLinha = 0;
            return bufferLinha[posicaoLinha++];
        } else {
            flag_EOF = TRUE;
            return EOF;
        }
    } else {
        return bufferLinha[posicaoLinha++];
    }
}

static void devolverProximoChar(void) {
    if (!flag_EOF) posicaoLinha--;
}

static TokenType buscarPalavraReservada(char *s) {
    for (int i = 0; i < MAXRESERVED; i++)
        if (!strcmp(s, palavrasReservadas[i].texto))
            return palavrasReservadas[i].token;
    return ID;
}

TokenType getToken(void) {
    int indiceStringToken = 0;
    TokenType tokenAtual = ERROR;
    TipoEstado estado = INICIO;
    int salvar;

    while (estado != CONCLUIDO) {
        int c = obterProximoChar();
        salvar = TRUE;

        switch (estado) {

        case INICIO:
            if (isdigit(c)) {
                estado = EMNUM;
            }
            else if (isalpha(c)) {
                estado = EMID;
            }
            else if (c == '=') {
                estado = EMATRIBUICAO;
            }
            else if (c == '!') {
                estado = EMNE;
            }
            else if (c == '<') {
                estado = EMLE;
            }
            else if (c == '>') {
                estado = EMGE;
            }
            else if ((c == ' ') || (c == '\t') || (c == '\n')) {
                salvar = FALSE;
            }
            else if (c == '/') {
                salvar = FALSE;
                int c2 = obterProximoChar();
                if (c2 == '*') {
                    estado = EMCOMENTARIO;
                    linhaInicioComentario = lineno;
                } else {
                    devolverProximoChar();
                    estado = CONCLUIDO;
                    tokenAtual = OVER;
                }
            }
            else {
                estado = CONCLUIDO;
                switch (c) {
                case EOF:
                    salvar = FALSE;
                    tokenAtual = ENDFILE;
                    break;
                case '+': tokenAtual = PLUS; break;
                case '-': tokenAtual = MINUS; break;
                case '*': tokenAtual = TIMES; break;
                case '(': tokenAtual = LPAREN; break;
                case ')': tokenAtual = RPAREN; break;
                case ';': tokenAtual = SEMI; break;
                case ',': tokenAtual = COMMA; break;
                case '[': tokenAtual = LBRACKET; break;
                case ']': tokenAtual = RBRACKET; break;
                case '{': tokenAtual = LBRACE; break;
                case '}': tokenAtual = RBRACE; break;
                default:
                    /* ERRO LÉXICO: caractere desconhecido */
                    fprintf(listing, "\nERRO LEXICO: '%c' - LINHA: %d\n", c, lineno);
                    Error = TRUE;
                    tokenAtual = ERROR;
                    break;
                }
            }
            break;

        case EMCOMENTARIO:
            salvar = FALSE;
            if (c == EOF) {
                estado = CONCLUIDO;
                tokenAtual = ENDFILE;
                fprintf(listing,
                        "\nERRO LEXICO: comentario nao fechado - LINHA: %d\n",
                        linhaInicioComentario);
                Error = TRUE;
            } else if (c == '*') {
                int c2 = obterProximoChar();
                if (c2 == '/') {
                    estado = INICIO;
                } else if (c2 == EOF) {
                    estado = CONCLUIDO;
                    tokenAtual = ENDFILE;
                    fprintf(listing,
                            "\nERRO LEXICO: comentario nao fechado - LINHA: %d\n",
                            linhaInicioComentario);
                    Error = TRUE;
                } else {
                    devolverProximoChar();
                }
            }
            break;

        case EMATRIBUICAO:
            estado = CONCLUIDO;
            if (c == '=') tokenAtual = EQ;
            else {
                devolverProximoChar();
                salvar = FALSE;
                tokenAtual = ASSIGN;
            }
            break;

        case EMNE:
            estado = CONCLUIDO;
            if (c == '=') tokenAtual = NE;
            else {
                /* ERRO LÉXICO: '!' sem '=' */
                devolverProximoChar();
                salvar = FALSE;
                fprintf(listing, "\nERRO LEXICO: '!' esperava '=' - LINHA: %d\n", lineno);
                Error = TRUE;
                tokenAtual = ERROR;
            }
            break;

        case EMLE:
            estado = CONCLUIDO;
            if (c == '=') tokenAtual = LE;
            else {
                devolverProximoChar();
                salvar = FALSE;
                tokenAtual = LT;
            }
            break;

        case EMGE:
            estado = CONCLUIDO;
            if (c == '=') tokenAtual = GE;
            else {
                devolverProximoChar();
                salvar = FALSE;
                tokenAtual = GT;
            }
            break;

        case EMNUM:
            /* Se apareceu letra logo após número => número mal-formado (10abc) => ERRO LÉXICO */
            if (isalpha(c)) {
                estado = EMNUMERRO;
                tokenAtual = ERROR;
                Error = TRUE;
                /* mantém salvar=TRUE para capturar o 'a' em stringToken */
            }
            else if (!isdigit(c)) {
                devolverProximoChar();
                salvar = FALSE;
                estado = CONCLUIDO;
                tokenAtual = NUM;
            }
            break;

        case EMNUMERRO:
            /* Consumir o resto do "token ruim" (letras/dígitos). Para ao encontrar delimitador. */
            if (!isalnum(c)) {
                devolverProximoChar();
                salvar = FALSE;
                estado = CONCLUIDO;

                /* stringToken já terá algo como "10abc" */
                /* OBS: a mensagem será emitida ao final, quando stringToken estiver fechado */
                tokenAtual = ERROR;
            }
            break;

        case EMID:
            /* ID no C-: letra (letra|digito)*  */
            if (!isalnum(c)) {
                devolverProximoChar();
                salvar = FALSE;
                estado = CONCLUIDO;
                tokenAtual = ID;
            }
            break;

        case CONCLUIDO:
        default:
            fprintf(listing, "\nERRO LEXICO: estado invalido\n");
            estado = CONCLUIDO;
            tokenAtual = ERROR;
            Error = TRUE;
            break;
        }

        if ((salvar) && (indiceStringToken <= MAXTOKENLEN)) {
            stringToken[indiceStringToken++] = (char)c;
        }

        if (estado == CONCLUIDO) {
            stringToken[indiceStringToken] = '\0';

            /* Se terminamos com ID, pode ser palavra reservada */
            if (tokenAtual == ID) {
                tokenAtual = buscarPalavraReservada(stringToken);
            }

            /* Se terminamos com ERROR por número mal-formado, reporta aqui com o lexema completo */
            if (tokenAtual == ERROR && indiceStringToken > 0 && isdigit((unsigned char)stringToken[0])) {
                /* pega casos como 10abc */
                int temLetra = 0;
                for (int i = 0; stringToken[i] != '\0'; i++) {
                    if (isalpha((unsigned char)stringToken[i])) { temLetra = 1; break; }
                }
                if (temLetra) {
                    fprintf(listing,
                            "\nERRO LEXICO: '%s' - LINHA: %d\n",
                            stringToken, lineno);
                }
            }
        }
    }

    if (TraceScan) {
        fprintf(listing, "\t%d: ", lineno);
        printToken(tokenAtual, stringToken);
    }

    return tokenAtual;
}