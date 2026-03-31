#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_OPERATOR,
    TOKEN_PUNCTUATION,
    TOKEN_COMMENT,
    TOKEN_UNKNOWN,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int line;
    int col;
} Token;

static const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default",
    "do", "double", "else", "enum", "extern", "float", "for", "goto",
    "if", "inline", "int", "long", "register", "restrict", "return",
    "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", NULL
};

typedef struct {
    const char *src;
    int pos;
    int line;
    int col;
} Lexer;

static void lexer_init(Lexer *l, const char *src) {
    l->src = src;
    l->pos = 0;
    l->line = 1;
    l->col = 1;
}

static char peek(Lexer *l) {
    return l->src[l->pos];
}

static char peek2(Lexer *l) {
    return l->src[l->pos] ? l->src[l->pos + 1] : '\0';
}

static char advance(Lexer *l) {
    char c = l->src[l->pos++];
    if (c == '\n') { l->line++; l->col = 1; }
    else l->col++;
    return c;
}

static int is_keyword(const char *s) {
    for (int i = 0; keywords[i]; i++)
        if (strcmp(keywords[i], s) == 0) return 1;
    return 0;
}

static const char *token_type_name(TokenType t) {
    switch (t) {
        case TOKEN_KEYWORD:     return "KEYWORD";
        case TOKEN_IDENTIFIER:  return "IDENTIFIER";
        case TOKEN_INTEGER:     return "INTEGER";
        case TOKEN_FLOAT:       return "FLOAT";
        case TOKEN_STRING:      return "STRING";
        case TOKEN_CHAR:        return "CHAR";
        case TOKEN_OPERATOR:    return "OPERATOR";
        case TOKEN_PUNCTUATION: return "PUNCTUATION";
        case TOKEN_COMMENT:     return "COMMENT";
        case TOKEN_UNKNOWN:     return "UNKNOWN";
        case TOKEN_EOF:         return "EOF";
        default:                return "?";
    }
}

Token next_token(Lexer *l) {
    Token tok = {TOKEN_UNKNOWN, "", l->line, l->col};

    while (peek(l) && isspace((unsigned char)peek(l)))
        advance(l);

    tok.line = l->line;
    tok.col  = l->col;

    char c = peek(l);

    if (!c) {
        tok.type = TOKEN_EOF;
        strcpy(tok.value, "EOF");
        return tok;
    }

    if (c == '/' && peek2(l) == '/') {
        int i = 0;
        while (peek(l) && peek(l) != '\n')
            tok.value[i++] = advance(l);
        tok.value[i] = '\0';
        tok.type = TOKEN_COMMENT;
        return tok;
    }

    if (c == '/' && peek2(l) == '*') {
        int i = 0;
        tok.value[i++] = advance(l);
        tok.value[i++] = advance(l);
        while (peek(l)) {
            char cc = advance(l);
            if (i < 254) tok.value[i++] = cc;
            if (cc == '*' && peek(l) == '/') {
                if (i < 254) tok.value[i++] = advance(l);
                else advance(l);
                break;
            }
        }
        tok.value[i] = '\0';
        tok.type = TOKEN_COMMENT;
        return tok;
    }

    if (c == '"') {
        int i = 0;
        tok.value[i++] = advance(l);
        while (peek(l) && peek(l) != '"') {
            if (peek(l) == '\\') tok.value[i++] = advance(l);
            if (i < 254) tok.value[i++] = advance(l);
            else advance(l);
        }
        if (peek(l)) tok.value[i++] = advance(l);
        tok.value[i] = '\0';
        tok.type = TOKEN_STRING;
        return tok;
    }

    if (c == '\'') {
        int i = 0;
        tok.value[i++] = advance(l);
        while (peek(l) && peek(l) != '\'') {
            if (peek(l) == '\\') tok.value[i++] = advance(l);
            if (i < 254) tok.value[i++] = advance(l);
            else advance(l);
        }
        if (peek(l)) tok.value[i++] = advance(l);
        tok.value[i] = '\0';
        tok.type = TOKEN_CHAR;
        return tok;
    }

    if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)peek2(l)))) {
        int i = 0;
        int is_float = 0;
        if (c == '0' && (peek2(l) == 'x' || peek2(l) == 'X')) {
            tok.value[i++] = advance(l);
            tok.value[i++] = advance(l);
            while (isxdigit((unsigned char)peek(l)))
                tok.value[i++] = advance(l);
        } else {
            while (isdigit((unsigned char)peek(l)))
                tok.value[i++] = advance(l);
            if (peek(l) == '.') { is_float = 1; tok.value[i++] = advance(l); }
            while (isdigit((unsigned char)peek(l)))
                tok.value[i++] = advance(l);
            if (peek(l) == 'e' || peek(l) == 'E') {
                is_float = 1;
                tok.value[i++] = advance(l);
                if (peek(l) == '+' || peek(l) == '-') tok.value[i++] = advance(l);
                while (isdigit((unsigned char)peek(l))) tok.value[i++] = advance(l);
            }
        }
        while (peek(l) == 'u' || peek(l) == 'U' || peek(l) == 'l' ||
               peek(l) == 'L' || peek(l) == 'f' || peek(l) == 'F') {
            char suf = peek(l);
            if (suf == 'f' || suf == 'F') is_float = 1;
            tok.value[i++] = advance(l);
        }
        tok.value[i] = '\0';
        tok.type = is_float ? TOKEN_FLOAT : TOKEN_INTEGER;
        return tok;
    }

    if (isalpha((unsigned char)c) || c == '_') {
        int i = 0;
        while (isalnum((unsigned char)peek(l)) || peek(l) == '_')
            if (i < 254) tok.value[i++] = advance(l);
            else advance(l);
        tok.value[i] = '\0';
        tok.type = is_keyword(tok.value) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
        return tok;
    }

    {
        const char *ops[] = {
            "<<=", ">>=", "...",
            "==", "!=", "<=", ">=", "&&", "||", "++", "--",
            "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
            "<<", ">>", "->", "::", NULL
        };
        for (int i = 0; ops[i]; i++) {
            int len = (int)strlen(ops[i]);
            if (strncmp(l->src + l->pos, ops[i], len) == 0) {
                for (int j = 0; j < len; j++) tok.value[j] = advance(l);
                tok.value[len] = '\0';
                tok.type = TOKEN_OPERATOR;
                return tok;
            }
        }
    }

    {
        const char *single_ops = "+-*/%&|^~!=<>?:.";
        if (strchr(single_ops, c)) {
            tok.value[0] = advance(l);
            tok.value[1] = '\0';
            tok.type = TOKEN_OPERATOR;
            return tok;
        }
    }

    {
        const char *puncts = "(){}[];,#@\\";
        if (strchr(puncts, c)) {
            tok.value[0] = advance(l);
            tok.value[1] = '\0';
            tok.type = TOKEN_PUNCTUATION;
            return tok;
        }
    }

    tok.value[0] = advance(l);
    tok.value[1] = '\0';
    tok.type = TOKEN_UNKNOWN;
    return tok;
}

typedef struct {
    int counts[11];
    int total;
    int lines;
} Stats;

static void print_stats(const Stats *s) {
    printf("\n=== Token Summary ===\n");
    printf("%-20s %s\n", "Token Type", "Count");
    printf("%-20s %s\n", "--------------------", "-----");
    TokenType order[] = {
        TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_INTEGER, TOKEN_FLOAT,
        TOKEN_STRING, TOKEN_CHAR, TOKEN_OPERATOR, TOKEN_PUNCTUATION,
        TOKEN_COMMENT, TOKEN_UNKNOWN
    };
    for (int i = 0; i < 10; i++) {
        if (s->counts[order[i]])
            printf("%-20s %d\n", token_type_name(order[i]), s->counts[order[i]]);
    }
    printf("%-20s %d\n", "--------------------", 0);
    printf("%-20s %d\n", "TOTAL (excl. EOF)", s->total);
    printf("%-20s %d\n", "Source lines", s->lines);
}

static void tokenize_and_print(const char *src, int show_comments) {
    Lexer l;
    lexer_init(&l, src);

    Stats s = {0};

    for (int i = 0; src[i]; i++)
        if (src[i] == '\n') s.lines++;
    if (s.lines == 0 && src[0]) s.lines = 1;

    printf("%-15s %-30s %s\n", "TYPE", "VALUE", "LINE:COL");
    printf("%-15s %-30s %s\n",
           "---------------", "------------------------------", "--------");

    Token tok;
    do {
        tok = next_token(&l);
        if (tok.type == TOKEN_EOF) break;

        if (!show_comments && tok.type == TOKEN_COMMENT) {
            s.counts[TOKEN_COMMENT]++;
            s.total++;
            continue;
        }

        char display[34];
        if (strlen(tok.value) > 30) {
            strncpy(display, tok.value, 27);
            strcpy(display + 27, "...");
        } else {
            strcpy(display, tok.value);
        }

        printf("%-15s %-30s %d:%d\n",
               token_type_name(tok.type), display, tok.line, tok.col);

        s.counts[tok.type]++;
        s.total++;
    } while (1);

    s.counts[TOKEN_EOF]++;

    print_stats(&s);
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    char *buf = (char *)malloc(sz + 1);
    if (!buf) {
        fprintf(stderr, "Error: out of memory\n");
        fclose(f);
        return NULL;
    }
    size_t rd = fread(buf, 1, sz, f);
    buf[rd] = '\0';
    fclose(f);
    return buf;
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <file.c>\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -c   Show comment tokens in output (hidden by default)\n");
    fprintf(stderr, "  -h   Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *filepath = NULL;
    int show_comments = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            show_comments = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            filepath = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (!filepath) {
        fprintf(stderr, "Error: no input file specified.\n");
        usage(argv[0]);
        return 1;
    }

    char *src = read_file(filepath);
    if (!src) return 1;

    printf("=== C Lexical Parser ===\n");
    printf("File : %s\n\n", filepath);

    tokenize_and_print(src, show_comments);

    free(src);
    return 0;
}