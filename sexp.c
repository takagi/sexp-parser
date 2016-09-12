#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*
 * bool
 */

typedef unsigned char bool;

enum {
    true  = 1,
    false = 0
};


/*
 * reader
 */

typedef struct {
    const char *str;
    const char *p;
} reader_t;

// Stack allocation interface.
void init_reader(reader_t *reader, const char *str)
{
    reader->str = str;
    reader->p = str;
}

// Heap allocation interface.
reader_t *create_reader(const char *str)
{
    reader_t *reader = (reader_t *)malloc(sizeof(reader_t));
    if (!reader) exit(-1);

    reader->str = str;
    reader->p = str;

    return reader;
}

void destroy_reader(reader_t *reader)
{
    free(reader);
}

char reader_head(reader_t *reader)
{
    return *reader->p;
}

char reader_next(reader_t *reader)
{
    if (*reader->p)
        return *reader->p++;
    else
        return *reader->p;
}


/*
 * sexp
 */

enum {
  TYPE_NIL    = 1,
  TYPE_SYMBOL = 2,
  TYPE_STRING = 3,
  TYPE_PAIR   = 4
};

typedef struct {
    unsigned char tt;
} sexp_t;

void destroy_sexp(sexp_t *);

void print_sexp(sexp_t *);


/*
 * symbol
 */

typedef struct {
    unsigned char tt;
    const char *name;
} symbol_t;

bool is_symbol(sexp_t *sexp)
{
    return sexp->tt == TYPE_SYMBOL;
}

const char *symbol_name(symbol_t *symbol)
{
    return symbol->name;
}

sexp_t *create_symbol(const char *name)
{
    symbol_t *symbol = (symbol_t *)malloc(sizeof(symbol_t));
    if (!symbol) exit(-1);

    symbol->tt = TYPE_SYMBOL;
    symbol->name = strdup(name);

    return (sexp_t *)symbol;
}

void destroy_symbol(symbol_t *symbol)
{
    free((char *)symbol->name);
    free(symbol);
}

void print_symbol(symbol_t *symbol)
{
    printf("%s", symbol->name);
}


/*
 * string
 */

typedef struct {
    unsigned char tt;
    const char *str;
} string_t;

bool is_string(sexp_t *sexp)
{
    return sexp->tt == TYPE_STRING;
}

const char *string_str(string_t *string)
{
    return string->str;
}

sexp_t *create_string(const char *str)
{
    string_t *string = (string_t *)malloc(sizeof(string_t));
    if (!string) exit(-1);

    string->tt = TYPE_STRING;
    string->str = strdup(str);

    return (sexp_t *)string;
}


void destroy_string(string_t *string)
{
    free((char *)string->str);
    free(string);
}

void print_string(string_t *string)
{
    printf("\"%s\"", string->str);
}


/*
 * nil
 */

typedef struct {
    unsigned char tt;
} nil_t;

bool is_nil(sexp_t *sexp)
{
    return sexp->tt == TYPE_NIL;
}

sexp_t *create_nil()
{
    nil_t *nil = (nil_t *)malloc(sizeof(nil_t));
    if (!nil) exit(-1);

    nil->tt = TYPE_NIL;

    return (sexp_t *)nil;
}

void destroy_nil(nil_t *nil)
{
    free(nil);
}

void print_nil(nil_t *nil)
{
    printf("()");
}


/*
 * pair
 */

typedef struct {
    unsigned char tt;
    sexp_t *car;
    sexp_t *cdr;
} pair_t;

bool is_pair(sexp_t *sexp)
{
    return sexp->tt == TYPE_PAIR;
}

sexp_t *pair_car(pair_t *pair)
{
    return pair->car;
}

sexp_t *pair_cdr(pair_t *pair)
{
    return pair->cdr;
}

sexp_t *create_pair(sexp_t *car, sexp_t *cdr)
{
    pair_t *pair = (pair_t *)malloc(sizeof(pair_t));
    if (!pair) exit(-1);

    pair->tt = TYPE_PAIR;
    pair->car = car;
    pair->cdr = cdr;

    return (sexp_t *)pair;
}

void destroy_pair(pair_t *pair)
{
    destroy_sexp(pair->car);
    destroy_sexp(pair->cdr);
    free(pair);
}

void print_pair(pair_t *pair)
{
    printf("(");
begin:
    print_sexp(pair->car);
    if (is_nil(pair->cdr)) {
        printf(")");
    } else if (is_pair(pair->cdr)) {
        printf(" ");
        pair = (pair_t *)pair->cdr;
        goto begin;
    } else {
        printf(" . ");
        print_sexp(pair->cdr);
        printf(")");
    }
}


/*
 * sexp
 */

void destroy_sexp(sexp_t *sexp)
{
    switch (sexp->tt) {
    case TYPE_NIL:
        destroy_nil((nil_t *)sexp);
        return;
    case TYPE_SYMBOL:
        destroy_symbol((symbol_t *)sexp);
        return;
    case TYPE_STRING:
        destroy_string((string_t *)sexp);
        return;
    case TYPE_PAIR:
        destroy_pair((pair_t *)sexp);
        return;
    default:
        fprintf(stderr, "Invalid S-expression.\n");
        exit(-1);
    }
}

void print_sexp(sexp_t *sexp)
{
    switch (sexp->tt) {
    case TYPE_NIL:
        print_nil((nil_t *)sexp);
        return;
    case TYPE_SYMBOL:
        print_symbol((symbol_t *)sexp);
        return;
    case TYPE_STRING:
        print_string((string_t *)sexp);
        return;
    case TYPE_PAIR:
        print_pair((pair_t *)sexp);
        return;
    default:
        fprintf(stderr, "Invalid S-expression.\n");
        exit(-1);
    }
}


/*
 * parser
 */

bool is_whitespace(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

bool is_terminating(unsigned char c)
{
    return c == '"' || c == '\'' || c == '(' || c == ')'
        || c == ',' || c == ';'  || c == '`';
}

bool is_single_escape(unsigned char c)
{
    return c == '\\';
}

sexp_t *parse_symbol(reader_t *reader)
{
#define ADD_BUF(_buf, _i, _c) do { \
        _buf[_i++] = _c; \
        if (_i == 256) { \
            fprintf(stderr, "Too long token: %s...\n", _buf); \
            exit(-1); \
        } \
    } while(0)

    char buf[256] = {0};
    int i = 0;
    char c;
    bool is_escaped = false;

    while ((c=reader_head(reader))) {

        if (is_escaped) {
            ADD_BUF(buf, i, c);
            reader_next(reader);
            is_escaped = false;
            continue;
        }

        if (is_whitespace(c))
            return create_symbol(buf);

        if (is_terminating(c))
            return create_symbol(buf);

        if (is_single_escape(c)) {
            reader_next(reader);
            is_escaped = true;
            continue;
        }

        ADD_BUF(buf, i, c);
        reader_next(reader);
    }

    return create_symbol(buf);

#undef ADD_BUF
}

void nreverse(sexp_t **psexp)
{
    sexp_t *prev = create_nil();
    sexp_t *current = *psexp;
    sexp_t *next;

    while(!is_nil(current)) {
        if (!is_pair(current)) {
            fprintf(stderr, "Not list.");
            exit(-1);
        }
        next = ((pair_t *)current)->cdr;
        ((pair_t *)current)->cdr = prev;
        prev = current;
        current = next;
    }
    destroy_sexp(current);
    *psexp = prev;
}

sexp_t *parse_string(reader_t *reader)
{
#define ADD_BUF(_buf, _i, _c) do { \
        _buf[_i++] = _c; \
        if (_i == 256) { \
            fprintf(stderr, "Too long token: %s...\n", _buf); \
            exit(-1); \
        } \
    } while(0)

    char buf[256] = {0};
    int i = 0;
    char c;
    bool is_escaped = false;

    while ((c=reader_head(reader))) {

        if (is_escaped) {
            ADD_BUF(buf, i, c);
            reader_next(reader);
            is_escaped = false;
            continue;
        }

        if (is_single_escape(c)) {
            reader_next(reader);
            is_escaped = true;
            continue;
        }

        if (c == '"') {
            reader_next(reader);
            return create_string(buf);
        }

        ADD_BUF(buf, i, c);
        reader_next(reader);
    }

    fprintf(stderr, "End of file.\n");
    exit(-1);

#undef ADD_BUF    
}

sexp_t *parse_list(reader_t *reader)
{
    sexp_t *list = create_nil();
    char c;

    while ((c=reader_head(reader))) {

        if (is_whitespace(c)) {
            reader_next(reader);
            continue;
        }

        if (c == '(') {
            reader_next(reader);
            list = create_pair(parse_list(reader), list);
            continue;
        }

        if (c == ')') {
            reader_next(reader);
            nreverse(&list);
            return list;
        }

        if (c == '"') {
            reader_next(reader);
            list = create_pair(parse_string(reader), list);
            continue;
        }

        list = create_pair(parse_symbol(reader), list);
    }

    fprintf(stderr, "End of file.\n");
    exit(-1);
}

sexp_t *parse(reader_t *reader)
{
    char c;

    while ((c=reader_head(reader))) {

        if (is_whitespace(c)) {
            reader_next(reader);
            continue;
        }

        if (c == '(') {
            reader_next(reader);
            return parse_list(reader);
        }

        if (c == ')') {
            fprintf(stderr, "Unmatched close parenthesis.\n");
            exit(-1);
        }

        if (c == '"') {
            reader_next(reader);
            return parse_string(reader);
        }

        return parse_symbol(reader);
    }

    return create_nil();
}


/*
 * main
 */

int main()
{
    reader_t reader;
    init_reader(&reader, " (ab\\(c de (f g) \"hi\\\"j\")");

    sexp_t *sexp = parse(&reader);

    print_sexp(sexp);
    printf("\n");
    
    destroy_sexp(sexp);
    sexp = NULL;

    return 0;
}
